#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "tf2/exceptions.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

#include "tf2/LinearMath/Quaternion.h"
#include "tf2/LinearMath/Matrix3x3.h"

#include "px4_msgs/msg/vehicle_command.hpp"
#include "px4_msgs/msg/trajectory_setpoint.hpp"

using namespace std::chrono_literals;

class DroneTFListener : public rclcpp::Node
{
public:
  DroneTFListener()
  : Node("drone_tf_listener"), is_landing_(false), landing_start_time_(0)
  {
    // Буфер для хранения трансформаций
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // Таймер для более частого обновления (10 Гц)
    timer_ = this->create_wall_timer(100ms, std::bind(&DroneTFListener::on_timer, this));

    // Пороги для центрирования
    lateral_threshold_ = 0.05;  // 5 см - более точное центрирование
    yaw_threshold_ = 0.1;       // ~5.7 градусов
    
    // Высота для начала посадки и завершения
    start_landing_height_ = 2.0;    // Начинаем посадку с 2 метров
    final_landing_height_ = 0.3;    // Финальная посадка с 30 см
    
    // Коэффициенты П-регулятора для плавного движения
    kp_xy_ = 0.8;    // Коэффициент для движения по X/Y
    kp_z_ = 0.4;     // Коэффициент для движения по Z (осторожнее)
    kp_yaw_ = 0.5;   // Коэффициент для поворота

    // Публикаторы для управления
    command_publisher_ = this->create_publisher<px4_msgs::msg::VehicleCommand>("/fmu/in/vehicle_command", 10);
    setpoint_publisher_ = this->create_publisher<px4_msgs::msg::TrajectorySetpoint>("/fmu/in/trajectory_setpoint", 10);
    
    RCLCPP_INFO(this->get_logger(), "Дрон готов к поиску и посадке по метке");
  }

private:
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};
  rclcpp::TimerBase::SharedPtr timer_{nullptr};
  rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr command_publisher_;
  rclcpp::Publisher<px4_msgs::msg::TrajectorySetpoint>::SharedPtr setpoint_publisher_;

  // Параметры алгоритма
  double lateral_threshold_;
  double yaw_threshold_;
  double start_landing_height_;
  double final_landing_height_;
  double kp_xy_;
  double kp_z_;
  double kp_yaw_;
  
  // Состояние системы
  bool is_landing_;
  int64_t landing_start_time_;

  void on_timer()
  {
    std::string from_frame = "camera_link";
    std::string to_frame = "tag36h11:0";

    geometry_msgs::msg::TransformStamped transform;
    try {
      transform = tf_buffer_->lookupTransform(from_frame, to_frame, tf2::TimePointZero);
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                          "Метка не обнаружена: %s", ex.what());
      return;
    }

    // Извлечение координат метки относительно камеры
    double x = transform.transform.translation.x;  // Право/лево (+ вправо)
    double y = transform.transform.translation.y;  // Вверх/вниз (+ вниз)  
    double z = transform.transform.translation.z;  // Вперед/назад (+ вперед) - ЭТО НАША ВЫСОТА!

    // Извлечение ориентации
    tf2::Quaternion q(
      transform.transform.rotation.x,
      transform.transform.rotation.y,
      transform.transform.rotation.z,
      transform.transform.rotation.w
    );

    tf2::Matrix3x3 m(q);
    double roll, pitch, yaw;
    m.getRPY(roll, pitch, yaw);

    // АЛГОРИТМ ПОСАДКИ
    execute_landing_algorithm(x, y, z, yaw);
  }

  void execute_landing_algorithm(double x, double y, double z, double yaw)
  {
    // 1. ПРОВЕРКА ВЫСОТЫ ДЛЯ НАЧАЛА ПОСАДКИ
    if (z > start_landing_height_ && !is_landing_) {
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 3000,
                          "Высота слишком большая (%.2f м). Приблизьтесь к метке", z);
      return;
    }

    // 2. АКТИВАЦИЯ РЕЖИМА ПОСАДКИ
    if (!is_landing_ && z <= start_landing_height_) {
      is_landing_ = true;
      landing_start_time_ = this->get_clock()->now().nanoseconds();
      RCLCPP_INFO(this->get_logger(), "Начинаем процедуру посадки! Текущая высота: %.2f м", z);
    }

    // 3. РАСЧЕТ ОШИБОК (П-регулятор)
    double error_x = -x;  // Отрицаем, т.к. если метка справа (x+), нужно двигаться влево
    double error_y = -y;  // Отрицаем, т.к. если метка внизу (y+), нужно двигаться назад  
    double error_z = z;   // Высота - чем меньше, тем лучше
    double error_yaw = -yaw;  // Отрицаем для корректного направления поворота

    // 4. РАСЧЕТ СКОРОСТЕЙ
    double vx = 0.0, vy = 0.0, vz = 0.0, yaw_rate = 0.0;

    // Центрирование по X/Y (только если не очень близко к земле)
    if (z > 0.5) {
      vx = kp_xy_ * error_y;  // Ось X дрона - вперед/назад
      vy = kp_xy_ * error_x;  // Ось Y дрона - влево/вправо
    }

    // Центрирование по рысканию
    if (std::abs(error_yaw) > yaw_threshold_) {
      yaw_rate = kp_yaw_ * error_yaw;
    }

    // Управление высотой (плавное снижение)
    if (z > final_landing_height_) {
      // Плавное снижение с коррекцией положения
      vz = -0.3 * kp_z_;  // Медленное снижение
    } else {
      // Близко к земле - только горизонтальная коррекция
      vz = -0.1 * kp_z_;  // Очень медленное снижение
    }

    // 5. ОГРАНИЧЕНИЕ СКОРОСТЕЙ (безопасность)
    vx = std::clamp(vx, -1.0, 1.0);
    vy = std::clamp(vy, -1.0, 1.0);
    vz = std::clamp(vz, -0.5, 0.5);
    yaw_rate = std::clamp(yaw_rate, -0.5, 0.5);

    // 6. ПРОВЕРКА УСЛОВИЙ ДЛЯ ФИНАЛЬНОЙ ПОСАДКИ
    bool is_centered = (std::abs(x) < lateral_threshold_) && 
                       (std::abs(y) < lateral_threshold_) &&
                       (std::abs(yaw) < yaw_threshold_);
    
    bool is_low = z <= final_landing_height_;

    // 7. ОТПРАВКА КОМАНД
    if (is_centered && is_low) {
      // Идеально отцентрирован над меткой на низкой высоте - ФИНАЛЬНАЯ ПОСАДКА
      send_land_command();
      RCLCPP_INFO(this->get_logger(), "ФИНАЛЬНАЯ ПОСАДКА! Высота: %.2f м", z);
    } else {
      // Отправка команд движения
      send_velocity_command(vx, vy, vz, yaw_rate);
      
      // Логирование каждые 2 секунды
      RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                          "Движение: vx=%.2f, vy=%.2f, vz=%.2f, yaw=%.2f | Ошибка: x=%.2f, y=%.2f, z=%.2f, yaw=%.2f",
                          vx, vy, vz, yaw_rate, x, y, z, yaw);
    }
  }

  void send_velocity_command(double vx, double vy, double vz, double yaw_rate)
  {
    // Используем TrajectorySetpoint для плавного управления
    auto msg = px4_msgs::msg::TrajectorySetpoint();
    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;
    
    // Скорости в локальной системе координат (NED)
    msg.velocity[0] = vx;  // Север (вперед)
    msg.velocity[1] = vy;  // Восток (вправо) 
    msg.velocity[2] = vz;  // Вниз (отрицательно = вверх)
    
    msg.yawspeed = yaw_rate;  // Скорость поворота
    
    // Флаги указывают, что используем velocity control
    msg.velocity_valid = true;
    msg.yawspeed_valid = true;

    setpoint_publisher_->publish(msg);
  }

  void send_land_command()
  {
    auto msg = px4_msgs::msg::VehicleCommand();
    msg.timestamp = this->get_clock()->now().nanoseconds() / 1000;
    msg.command = px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND;
    msg.param1 = 0.0f;  // Минимальная скорость спуска
    msg.param2 = 0.0f;
    msg.param3 = 0.0f;
    msg.param4 = std::numeric_limits<float>::quiet_NaN();  // Yaw
    msg.param5 = std::numeric_limits<float>::quiet_NaN();  // Широта (NaN = текущая)
    msg.param6 = std::numeric_limits<float>::quiet_NaN();  // Долгота (NaN = текущая)
    msg.param7 = std::numeric_limits<float>::quiet_NaN();  // Высота
    msg.target_system = 1;
    msg.target_component = 1;
    msg.source_system = 255;
    msg.source_component = 0;
    msg.from_external = true;
    msg.confirmation = 0;

    command_publisher_->publish(msg);
    RCLCPP_INFO(this->get_logger(), "Команда посадки отправлена!");
  }
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DroneTFListener>());
  rclcpp::shutdown();
  return 0;
}