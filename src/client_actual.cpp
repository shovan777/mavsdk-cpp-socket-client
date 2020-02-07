
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <sstream>

// Header file of Socket.IO C ++ Client
// Socket.IO C++ Clientのヘッダファイル
#include <sio_client.h>

class SampleClient {
public:
  // Socket.IOのインスタンス
  // instance of Socket.IO
  sio::client client;
  sio::socket::ptr socket;

// Mutexes to synchronize the sio thread with the main thread
  // sioのスレッドとメインスレッドの同期をとるためのmutex類
  std::mutex sio_mutex;
  std::condition_variable_any sio_cond;

  // queue for storing sio messages
  // sioのメッセージを貯めるためのキュー
  std::queue<sio::message::ptr> sio_queue;
  
  bool is_connected = false;

// Event listener called when disconnecting
  // 切断時に呼び出されるイベントリスナ
  void on_close() {
    // Disconnected
    std::cout << "切断しました。" << std::endl;
    exit(EXIT_FAILURE);
  }

  
  // Event listener called when an error occurs
//エラー発生時に呼び出されるイベントリスナ
  void on_fail() {
    // There was an error.
    std::cout << "エラーがありました。" << std::endl;
    exit(EXIT_FAILURE);
  }

// Event listener called at connection
  // 接続時に呼び出されるイベントリスナ
  void on_open() {
    // There was an error....
    std::cout << "Connected" << std::endl;
    std::unique_lock<std::mutex> lock(sio_mutex);
    is_connected = true;
    // Wake up the main thread after the connection process is over
    // 接続処理が終わったのち、待っているメインスレッドを起こす
    sio_cond.notify_all();
  }


// Event listener for "run" command
  // "run"コマンドのイベントリスナ
  void on_run(sio::event& e) {
    std::unique_lock<std::mutex> lock(sio_mutex);
    sio_queue.push(e.get_message());

    // enqueue the event and wake up the waiting main thread
    // イベントをキューに登録し、待っているメインスレッドを起こす
    sio_cond.notify_all();
  }

// Main processing
  // メイン処理
  void run( const std::string& name) {
    // Set listener for connection and error events
    // 接続およびエラーイベントのリスナを設定する
    client.set_close_listener(std::bind(&SampleClient::on_close, this));
    client.set_fail_listener(std::bind(&SampleClient::on_fail, this));
    client.set_open_listener(std::bind(&SampleClient::on_open, this));
    
    // Issue a connection request
    // 接続要求を出す
    client.connect("http://192.168.0.5:3000/");
    { 
      //  Wait until the connection process running on another thread ends
      // 別スレッドで動く接続処理が終わるまで待つ
      std::unique_lock<std::mutex> lock(sio_mutex);
      if (!is_connected) {
	sio_cond.wait(sio_mutex);
      }
    }
    
// Register a listener for the "run" command
    socket = client.socket();
    socket->on("run", std::bind(&SampleClient::on_run, this, std::placeholders::_1));

    {
      socket->emit("join");
    }

    while(true) {
      // If the event queue is empty, wait until the queue is refilled
      // イベントキューが空の場合、キューが補充されるまで待つ
      std::unique_lock<std::mutex> lock(sio_mutex);
      while (sio_queue.empty()) {
	sio_cond.wait(lock);
      }

// Retrieve the registered data from the event queue
      // イベントキューから登録されたデータを取り出す
      sio::message::ptr recv_data(sio_queue.front());
      std::stringstream output;
      char buf[1024];
      
      FILE* fp = nullptr;
      // get the value of the command member of the object
      // objectのcommandメンバの値を取得する
      std::string command = recv_data->get_map().at("command")->get_string();
      std::cout << "run:" << command << std::endl;
      // Execute command and get the execution result as a string
      // commandを実行し、実行結果を文字列として取得する
      if ((fp = popen(command.c_str(), "r")) != nullptr) {
	while (!feof(fp)) {
	  size_t len = fread(buf, 1, sizeof(buf), fp);
	  output << std::string(buf, len);
	}

      } else {
        // If an error is detected, get the error message
	// エラーを検出した場合はエラーメッセージを取得する
	output << strerror(errno);
      }
      
      pclose(fp);
      
      sio::message::ptr send_data(sio::object_message::create());
      std::map<std::string, sio::message::ptr>& map = send_data->get_map();

// Set the execution result of the command to the output of the object
      // コマンドの実行結果をobjectのoutputに設定する
      map.insert(std::make_pair("output", sio::string_message::create(output.str())));

      // send sio :: message to server
      // sio::messageをサーバに送る
      socket->emit("reply", send_data);

      // remove the processed event from the queue
      // 処理が終わったイベントをキューから取り除く
      sio_queue.pop();
    }
  }
};

int main(int argc, char* argv[]) {
  // 引数を１つとり、名前とする
  // Take one argument and name it

  if (argc != 2) {
    // Specify the connection destination and name as arguments.
    std::cerr << "接続先と名前を引数に指定してください。" << std::endl;
    exit(EXIT_FAILURE);
  }

  SampleClient client;
  client.run(argv[1]);

  return EXIT_SUCCESS;
}
