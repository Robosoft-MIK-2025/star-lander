// Copyright 2023 Your Name (adapted from ROS2 tutorials)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// ...

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "tf2/exceptions.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "geometry_msgs/msg/transform_stamped.hpp"

#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"   // Для tf2::Matrix3x3 и getRPY

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

    // Пороги для команд (можно сделать параметрами)
    lateral_threshold_ = 0.1;  // м для x/y
    depth_threshold_min_ = 0.5;  // м, слишком близко
    depth_threshold_max_ = 1.0;  // м, слишком далеко

    roll_treshold = 0.2;
    pitch_treshold = 0.1;
    yaw_treshold = 0.1;
  }

private:
  void on_timer()
  {
    std::string from_frame = "camera_link";  // Фрейм камеры
    std::string to_frame = "tag36h11:0";     // Фрейм AprilTag

    geometry_msgs::msg::TransformStamped transform;
    try {
      // Запрос трансформации (последняя доступная)
      transform = tf_buffer_->lookupTransform(from_frame, to_frame, tf2::TimePointZero);
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN(this->get_logger(), "Не удалось получить трансформацию от %s к %s: %s",
                  from_frame.c_str(), to_frame.c_str(), ex.what());
      return;
    }

    // Извлечение translation
    double x = transform.transform.translation.x;
    double y = transform.transform.translation.y;
    double z = transform.transform.translation.z;

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
    RCLCPP_INFO(this->get_logger(), "Трансформация: x=%.2f, y=%.2f, z=%.2f. Команды: %s\nПоворот дрона: x=%.2f, y=%.2f, z=%.2f, w=%.2f, roll=%.2f, pitch=%.2f, yaw=%.2f. Команды: %s",
    x, y, z, translations.c_str(), rotate_tf2.x, rotate_tf2.y, rotate_tf2.z, rotate_tf2.w, roll, pitch, yaw, rotations.c_str());
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

    if (std::abs(roll + 3.0) > roll_treshold) {
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

  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};
  rclcpp::TimerBase::SharedPtr timer_{nullptr};

  double lateral_threshold_;
  double depth_threshold_min_;
  double depth_threshold_max_;
  double roll_treshold;
  double pitch_treshold;
  double yaw_treshold;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DroneTFListener>());
  rclcpp::shutdown();
  return 0;
}