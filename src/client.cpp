//
// Copyright (c) Prokura Innovations. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
#include <chrono>
#include <cstdint>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <sstream>
#include <thread>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

// Header file of Socket.IO C ++ Client
// Socket.IO C++ Clientのヘッダファイル
#include <sio_client.h>

using namespace mavsdk;
using namespace std::this_thread;
using namespace std::chrono;

#define ERROR_CONSOLE_TEXT "\033[31m" // Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" // Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m" // Restore normal console colour

void usage(std::string bin_name)
{
    std::cout << NORMAL_CONSOLE_TEXT << "Usage : " << bin_name << " <connection_url>" << std::endl
              << "Connection URL format should be :" << std::endl
              << " For TCP : tcp://[server_host][:server_port]" << std::endl
              << " For UDP : udp://[bind_host][:bind_port]" << std::endl
              << " For Serial : serial:///path/to/serial/dev[:baudrate]" << std::endl
              << "For example, to connect to the simulator use URL: udp://:14540" << std::endl;
}

void component_discovered(ComponentType component_type)
{
    std::cout << NORMAL_CONSOLE_TEXT << "Discovered a component with type "
              << unsigned(component_type) << std::endl;
}

class SampleClient
{
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
    void on_close()
    {
        // Disconnected
        std::cout << "切断しました。" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Event listener called when an error occurs
    //エラー発生時に呼び出されるイベントリスナ
    void on_fail()
    {
        // There was an error.
        std::cout << "エラーがありました。" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Event listener called at connection
    // 接続時に呼び出されるイベントリスナ
    void on_open()
    {
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
    void on_run(sio::event &e)
    {
        std::unique_lock<std::mutex> lock(sio_mutex);
        sio_queue.push(e.get_message());

        // enqueue the event and wake up the waiting main thread
        // イベントをキューに登録し、待っているメインスレッドを起こす
        sio_cond.notify_all();
    }

    // Main processing
    // メイン処理
    void run()
    {
        // Set listener for connection and error events
        // 接続およびエラーイベントのリスナを設定する
        client.set_close_listener(std::bind(&SampleClient::on_close, this));
        client.set_fail_listener(std::bind(&SampleClient::on_fail, this));
        client.set_open_listener(std::bind(&SampleClient::on_open, this));

        // Issue a connection request
        // 接続要求を出す
        client.connect("http://nicwebpage.herokuapp.com/");
        {
            //  Wait until the connection process running on another thread ends
            // 別スレッドで動く接続処理が終わるまで待つ
            std::unique_lock<std::mutex> lock(sio_mutex);
            if (!is_connected)
            {
                sio_cond.wait(sio_mutex);
            }
        }

        // Register a listener for the "run" command
        socket = client.socket("JT601");
        socket->emit("joinPi");

        // while (true)
        // {
        //     // If the event queue is empty, wait until the queue is refilled
        //     // イベントキューが空の場合、キューが補充されるまで待つ
        //     // std::unique_lock<std::mutex> lock(sio_mutex);
        //     // while (sio_queue.empty())
        //     // {
        //     //     sio_cond.wait(lock);
        //     // }

        //     // // Retrieve the registered data from the event queue
        //     // // イベントキューから登録されたデータを取り出す
        //     // sio::message::ptr recv_data(sio_queue.front());
        //     char output[] = "hello world";
        //     // char buf[1024];

        //     // FILE *fp = nullptr;
        //     // get the value of the command member of the object
        //     // objectのcommandメンバの値を取得する
        //     // std::string command = recv_data->get_map().at("command")->get_string();
        //     std::cout << "run:" << output << std::endl;
        //     // Execute command and get the execution result as a string
        //     // commandを実行し、実行結果を文字列として取得する
        //     // if ((fp = popen(command.c_str(), "r")) != nullptr)
        //     // {
        //     //     while (!feof(fp))
        //     //     {
        //     //         size_t len = fread(buf, 1, sizeof(buf), fp);
        //     //         output << std::string(buf, len);
        //     //     }
        //     // }
        //     // else
        //     // {
        //     //     // If an error is detected, get the error message
        //     //     // エラーを検出した場合はエラーメッセージを取得する
        //     //     output << strerror(errno);
        //     // }

        //     // pclose(fp);

        //     sio::message::ptr send_data(sio::object_message::create());
        //     std::map<std::string, sio::message::ptr> &map = send_data->get_map();

        //     // Set the execution result of the command to the output of the object
        //     // コマンドの実行結果をobjectのoutputに設定する
        //     map.insert(std::make_pair("output", sio::string_message::create(output)));

        //     // send sio :: message to server
        //     // sio::messageをサーバに送る
        //     socket->emit("data", send_data);

        //     // remove the processed event from the queue
        //     // 処理が終わったイベントをキューから取り除く
        //     // sio_queue.pop();
        // }
    }
};

SampleClient client;
int main(int argc, char** argv)
{

    Mavsdk dc;
    std::string connection_url;
    ConnectionResult connection_result;

    // SampleClient client;
    client.run();

    bool discovered_system = false;
    if (argc == 2) {
        connection_url = argv[1];
        connection_result = dc.add_any_connection(connection_url);
    } else {
        usage(argv[0]);
        return 1;
    }

    if (connection_result != ConnectionResult::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT
                  << "Connection failed: " << connection_result_str(connection_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // We don't need to specify the UUID if it's only one system anyway.
    // If there were multiple, we could specify it with:
    // dc.system(uint64_t uuid);
    System& system = dc.system();

    std::cout << "Waiting to discover system..." << std::endl;
    dc.register_on_discover([&discovered_system](uint64_t uuid) {
        std::cout << "Discovered system with UUID: " << uuid << std::endl;
        discovered_system = true;
    });

    sleep_for(seconds(2));

    if (!discovered_system) {
        std::cout << ERROR_CONSOLE_TEXT << "No system found, exiting." << NORMAL_CONSOLE_TEXT
                  << std::endl;
        return 1;
    }

    // Register a callback so we get told when components (camera, gimbal) etc
    // are found.
    system.register_component_discovered_callback(component_discovered);

    auto telemetry = std::make_shared<Telemetry>(system);
    auto action = std::make_shared<Action>(system);

    // We want to listen to the altitude of the drone at 1 Hz.
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);
    if (set_rate_result != Telemetry::Result::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT
                  << "Setting rate failed:" << Telemetry::result_str(set_rate_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // Set up callback to monitor altitude while the vehicle is in flight
    telemetry->position_async([](Telemetry::Position position) {
        std::cout << TELEMETRY_CONSOLE_TEXT // set to blue
                  << "Altitude: " << position.relative_altitude_m << " m"
                  << "Latitude: " << position.latitude_deg << " deg"
                  << "Longitude: " << position.longitude_deg << " m"
                  << NORMAL_CONSOLE_TEXT // set to default color again
                  << std::endl;

        sio::message::ptr send_data(sio::object_message::create());
        std::map<std::string, sio::message::ptr> &map = send_data->get_map();
        // char output = euler.pitch_deg;
        // char output[] = "hello world";        // Set the execution result of the command to the output of the object
        // コマンドの実行結果をobjectのoutputに設定する
        map.insert(std::make_pair("lat", sio::string_message::create(std::to_string(position.latitude_deg))));
        map.insert(std::make_pair("lng", sio::string_message::create(std::to_string(position.latitude_deg))));
        map.insert(std::make_pair("altr", sio::string_message::create(std::to_string(position.relative_altitude_m))));
        map.insert(std::make_pair("alt", sio::string_message::create(std::to_string(position.absolute_altitude_m))));
        map.insert(std::make_pair("conn", sio::string_message::create("True")));
        // map.insert(std::make_pair("conn", sio::string_message::create("True")));
        std::cout << "sending data ..." << std::endl;
        //           << map << std::endl;

        // send sio :: message to server
        // sio::messageをサーバに送る
        client.socket->emit("data", send_data);

    });
    // telemetry->attitude_euler_angle_async([](Telemetry::EulerAngle euler) {
    //     std::cout << TELEMETRY_CONSOLE_TEXT // set to blue
    //               << "Pitch: " << euler.pitch_deg << " deg"
    //               << "Roll: " << euler.roll_deg << " deg"
    //               << "Yaw: " << euler.yaw_deg <<" deg"
    //               << NORMAL_CONSOLE_TEXT // set to default color again
    //               << std::endl;


    //     sio::message::ptr send_data(sio::object_message::create());
    //     std::map<std::string, sio::message::ptr> &map = send_data->get_map();
    //     // char output = euler.pitch_deg;
    //     char output[] = "hello world";        // Set the execution result of the command to the output of the object
    //     // コマンドの実行結果をobjectのoutputに設定する
    //     map.insert(std::make_pair("output", sio::string_message::create(output)));
    //     // map.insert(std::make_pair("conn", sio::string_message::create("True")));
    //     std::cout << "sending data ..." << std::endl;
    //     //           << map << std::endl;

    //     // send sio :: message to server
    //     // sio::messageをサーバに送る
    //     client.socket->emit("data", send_data);

    // });

    // Check if vehicle is ready to arm
    while (telemetry->health_all_ok() != true) {
        std::cout << "Vehicle is getting ready to arm" << std::endl;
        sleep_for(seconds(1));
    }

    // Arm vehicle
    std::cout << "Arming..." << std::endl;
    const Action::Result arm_result = action->arm();

    if (arm_result != Action::Result::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT << "Arming failed:" << Action::result_str(arm_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // Take off
    std::cout << "Taking off..." << std::endl;
    const Action::Result takeoff_result = action->takeoff();
    if (takeoff_result != Action::Result::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT << "Takeoff failed:" << Action::result_str(takeoff_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // Let it hover for a bit before landing again.
    sleep_for(seconds(10));

    std::cout << "Landing..." << std::endl;
    const Action::Result land_result = action->land();
    if (land_result != Action::Result::SUCCESS) {
        std::cout << ERROR_CONSOLE_TEXT << "Land failed:" << Action::result_str(land_result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        return 1;
    }

    // Check if vehicle is still in air
    while (telemetry->in_air()) {
        std::cout << "Vehicle is landing..." << std::endl;
        sleep_for(seconds(1));
    }
    std::cout << "Landed!" << std::endl;

    // We are relying on auto-disarming but let's keep watching the telemetry for a bit longer.
    sleep_for(seconds(3));
    std::cout << "Finished..." << std::endl;

    //sio code is here
    // 引数を１つとり、名前とする
    // Take one argument and name it

    //   if (argc != 2) {
    //     // Specify the connection destination and name as arguments.
    //     std::cerr << "接続先と名前を引数に指定してください。" << std::endl;
    //     exit(EXIT_FAILURE);
    //   }

    //   client.run(argv[1]);

    return EXIT_SUCCESS;
}
