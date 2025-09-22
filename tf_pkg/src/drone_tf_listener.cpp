
// Copyright 2023 Your Name (adapted from ROS2 tutorials)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// ...

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <limits>  // Для std::numeric_limits (NaN)

#include "rclcpp/rclcpp.hpp"
#include "tf2/exceptions.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "geometry_msgs/msg/transform_stamped.hpp"

#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"   // Для tf2::Matrix3x3 и getRPY

#include "px4_msgs/msg/vehicle_command.hpp"
#include "px4_msgs/msg/vehicle_global_position.hpp"

#include "rclcpp/qos.hpp"  // Для QoS

using namespace std::chrono_literals;

class DroneTFListener : public rclcpp::Node
{

public:
  DroneTFListener()
  : Node("drone_tf_listener")
  {
    // Буфер для хранения трансформаций (по умолчанию 10 сек)
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // Таймер для запроса трансформаций каждую секунду
    timer_ = this->create_wall_timer(1s, std::bind(&DroneTFListener::on_timer, this));

    // TODO: Сделать параметрами ros2 ???!!!!

    // Пороги для команд перемещения (можно сделать параметрами)
    lateral_threshold_ = 0.1;  // м для x/y
    depth_threshold_min_ = 0.5;  // м, слишком близко
    depth_threshold_max_ = 1.0;  // м, слишком далеко

    // пороги для команд поворота
    roll_treshold = 0.2;
    pitch_treshold = 0.1;
    yaw_treshold = 0.1;

    // END TODO

    command_publisher_ = this->create_publisher<px4_msgs::msg::VehicleCommand>("/fmu/in/vehicle_command", 10);

    global_pos_sub_ = this->create_subscription<px4_msgs::msg::VehicleGlobalPosition>("/fmu/out/vehicle_global_position",
      rclcpp::SensorDataQoS(),
      std::bind(
        &DroneTFListener::global_pos_callback,
        this,
        std::placeholders::_1
      )
    );
  }

private:
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};
  rclcpp::TimerBase::SharedPtr timer_{nullptr};
  rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr command_publisher_;

  double lateral_threshold_;
  double depth_threshold_min_;
  double depth_threshold_max_;
  double roll_treshold;
  double pitch_treshold;
  double yaw_treshold;

  // Глобальная позиция дрона - подписка
  rclcpp::Subscription<px4_msgs::msg::VehicleGlobalPosition>::SharedPtr global_pos_sub_;
  // Сами координаты глобальной позиции
  long double current_lat_ = 0.0,
        current_lon_ = 0.0,
        current_alt_ = 0.0;

  bool isFirst = false;
  rclcpp::Time first;

  void on_timer()
  {
    std::string from_frame = "x500_vision_0/vision_link/vision";  // Фрейм камеры: camera_link
    std::string to_frame = "tag36h11:0";     // Фрейм AprilTag

    // Сначала проверяем, доступна ли TF (с timeout 100мс)
    // if (!tf_buffer_->canTransform(from_frame, to_frame, tf2::TimePointZero, tf2::durationFromSec(0.1))) {
    //   RCLCPP_WARN(this->get_logger(), "TF от %s к %s недоступна в течение 0.1с.", from_frame.c_str(), to_frame.c_str());
    //   return;
    // }


      if (!isFirst)
      {
        float desired_alt = std::numeric_limits<float>::quiet_NaN(); // Down positive in NED // current_alt_ - z
        float desired_yaw = 0.0f; // Или текущий + yaw (в deg)
        send_reposition_command(47.3979706, 8.546283, desired_alt, desired_yaw);
        isFirst = true;
        first = this->get_clock()->now();
      }
      
      try {
      // Запрос трансформации (последняя доступная)
      geometry_msgs::msg::TransformStamped transform;
    transform = tf_buffer_->lookupTransform(from_frame, to_frame, tf2::TimePointZero);

          // Извлечение translation
      long double x = transform.transform.translation.x;
      long double y = transform.transform.translation.y;
      long double z = transform.transform.translation.z;

      const auto& rotate_tf2 = transform.transform.rotation;

      tf2::Quaternion q(
        rotate_tf2.x,
        rotate_tf2.y,
        rotate_tf2.z,
        rotate_tf2.w
      );

      tf2::Matrix3x3 m(q);
      double roll, pitch, yaw;
      m.getRPY(roll, pitch, yaw);

      // Генерация команд
      std::string translations = generate_translations(x, y, z);
      std::string rotations = generate_rotations(roll, pitch, yaw);

      // Вывод команд (как будто инструкции дрону)
      // RCLCPP_INFO(this->get_logger(), "Трансформация: x=%.2f, y=%.2f, z=%.2f. Команды: %s\nПоворот дрона: x=%.2f, y=%.2f, z=%.2f, w=%.2f, roll=%.2f, pitch=%.2f, yaw=%.2f. Команды: %s", x, y, z, translations.c_str(), rotate_tf2.x, rotate_tf2.y, rotate_tf2.z, rotate_tf2.w, roll, pitch, yaw, rotations.c_str());

      // сокращённая версия (без лютого засера лога)
      RCLCPP_INFO(this->get_logger(), "Трансформация: Команды: %s\nПоворот дрона: Команды: %s", translations.c_str(),rotations.c_str());

      long double desired_lat = current_lat_ + (y / 111111.0L); // ~1м = 1/111111 deg lat (примерно)
      long double desired_lon = current_lon_ + (x / (111111.0L * static_cast<long double>(cos(current_lat_ * M_PI / 180)))); // Корректировка для lon
      float desired_alt = std::numeric_limits<float>::quiet_NaN(); // Down positive in NED // current_alt_ - z
      float desired_yaw = 0.0f; // Или текущий + yaw (в deg)
      RCLCPP_INFO(
        this->get_logger(), "current: %.10Lf %.10Lf %.10Lf\ntransform: %.10Lf %.10Lf %.10Lf\ndesired: %.10Lf %.10Lf %.10f\n",
        current_lat_,
        current_lon_,
        current_alt_,
        x, y, z,
        desired_lat,
        desired_lon,
        desired_alt
      );
      // send_reposition_command(desired_lat, desired_lon, desired_alt, desired_yaw);
      // send_reposition_command(47.3979706, 8.546283, desired_alt, desired_yaw);
      RCLCPP_INFO(this->get_logger(), "Отправлена команда перемещения для центрирования.");
    }
    catch (const tf2::TransformException & ex) {
    }
      
    if ((this->get_clock()->now() - first).seconds() > 10.0)
    {
      send_land_command();
    }

    // Проверяем свежесть TF (stamp должен быть не старше 1с)
    // rclcpp::Time now = this->get_clock()->now();
    // rclcpp::Time tf_stamp(transform.header.stamp);
    // if ((now - tf_stamp).seconds() > 0.31) {
    //   RCLCPP_WARN(this->get_logger(), "TF от %s к %s устарела (stamp: %f сек назад). Пропускаем.", 
    //               from_frame.c_str(), to_frame.c_str(), (now - tf_stamp).seconds());
    // if ((now - tf_stamp).seconds() > 1) {
    //   // RCLCPP_WARN(this->get_logger(), "TF от %s к %s устарела (stamp: %f сек назад). Пропускаем.", 
    //               // from_frame.c_str(), to_frame.c_str(), (now - tf_stamp).seconds());

    //   // Отмена reposition: Отправляем hover (hold на месте)
    //   // крч говорим дрону зависнуть на месте
    //   send_hover_command();
    //   // send_hover_command();
    //   return;
    // }

    // Если центрировано, отправь посадку (на текущей позиции или с координатами)

  }


  std::string generate_translations(double x, double y, double z)
  {
    std::string cmd = "";

    // Для x (горизонталь: положительное - вправо)
    if (std::abs(x) > lateral_threshold_) {
      if (x > 0) {
        cmd += "правее; ";
      } else {
        cmd += "левее; ";
      }
    }

    // Для y (вертикаль: положительное - вниз)
    if (std::abs(y) > lateral_threshold_) {
      if (y > 0) {
        cmd += "вперёд; ";
      } else {
        cmd += "назад; ";
      }
    }

    // Для z (глубина: вперед/назад)
    if (z > depth_threshold_max_) {
      cmd += "ниже; ";
    } else if (z < depth_threshold_min_) {
      cmd += "выше; ";
    }

    if (cmd.empty()) {
      cmd = "на месте (цель центрирована)";
    } else {
      // Убрать trailing "; "
      cmd = cmd.substr(0, cmd.length() - 2);
    }

    return cmd;
  }

  std::string generate_rotations(double roll, double pitch, double yaw)
  {
    std::string cmd = "";

    if (std::abs(roll) > roll_treshold) { // + 3.0
      if (roll > 0) {
        cmd += "наклонись вперёд; ";
      } else {
        cmd += "наклонись назад; ";
      }
    }

    if (std::abs(pitch) > pitch_treshold) {
      if (pitch > 0) {
        cmd += "наклонись вправо; ";
      } else {
        cmd += "наклонись влево; ";
      }
    }

    if (std::abs(yaw) > yaw_treshold) {
      if (yaw > 0) {
        cmd += "повернись влево; ";
      } else {
        cmd += "повернись вправо; ";
      }
    }

    if (cmd.empty()) {
      cmd = "на месте (цель центрирована)";
    } else {
      // Убрать trailing "; "
      cmd = cmd.substr(0, cmd.length() - 2);
    }

    return cmd;
  }

  void send_land_command(double latitude = 0.0, double longitude = 0.0, float altitude = 0.0f)
  {
    auto msg = px4_msgs::msg::VehicleCommand();
    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;  // Timestamp в мкс
    msg.command = px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND;  // Команда посадки
    msg.param1 = 0.0f;  // Минимальная скорость спуска (0 для дефолта)
    msg.param2 = 0.0f;
    msg.param3 = 0.0f;
    msg.param4 = std::numeric_limits<float>::quiet_NaN();  // Yaw (NaN для игнора)
    msg.param5 = static_cast<float>(latitude);  // Широта (градусы)
    msg.param6 = static_cast<float>(longitude);  // Долгота (градусы)
    msg.param7 = altitude;  // Высота AMSL (метры)
    msg.target_system = 1;  // Твой дрон
    msg.target_component = 1;
    msg.source_system = 255;  // От ROS2
    msg.source_component = 0;
    msg.from_external = true;  // Внешняя команда
    msg.confirmation = 0;

    command_publisher_->publish(msg);
    RCLCPP_INFO(this->get_logger(), "Отправлена команда посадки на координаты: lat=%.6f, lon=%.6f, alt=%.2f", latitude, longitude, altitude);
  }

  void send_reposition_command(double latitude, double longitude, float altitude, float yaw) {
    auto msg = px4_msgs::msg::VehicleCommand();
    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;
    msg.command = px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_REPOSITION; // 192
    msg.param1 = 0.0f; // Flags (0 для default: reposition and hold)
    msg.param2 = 0.0f; // Loiter radius (0 ignore)
    msg.param3 = 0.0f; // Loiter direction
    msg.param4 = yaw; // Yaw (deg, NaN ignore)
    msg.param5 = static_cast<float>(latitude);
    msg.param6 = static_cast<float>(longitude);
    msg.param7 = altitude;
    msg.target_system = 1;
    msg.target_component = 1;
    msg.source_system = 255;
    msg.source_component = 0;
    msg.from_external = true;
    msg.confirmation = 0;
    command_publisher_->publish(msg);
    RCLCPP_INFO(this->get_logger(), "Отправлена команда REPOSITION: lat=%.6f, lon=%.6f, alt=%.2f, yaw=%.2f", latitude, longitude, altitude, yaw);
  }

  void send_hover_command() {
    double nan = std::numeric_limits<double>::quiet_NaN();
    float nan_f = std::numeric_limits<float>::quiet_NaN();
    send_reposition_command(nan, nan, nan_f, nan_f);  // NaN в lat/lon/alt/yaw — hold current
    RCLCPP_INFO(this->get_logger(), "Отправлена команда HOVER (hold на месте) из-за устаревшей TF.");
  }

  void global_pos_callback(const px4_msgs::msg::VehicleGlobalPosition::SharedPtr msg) {
    current_lat_ = msg->lat;
    current_lon_ = msg->lon;
    current_alt_ = msg->alt; // AMSL
  }
    // RCLCPP_INFO(this->get_logger(), "Получена глобальная позиция: lat=%.6f, lon=%.6f, alt=%.2f", current_lat_, current_lon_, current_alt_);
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DroneTFListener>());
  rclcpp::shutdown();
  return 0;
}