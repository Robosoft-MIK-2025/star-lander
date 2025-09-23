# **Проект "TO-DO"**  

## 🛠 **Установка и настройка**  

### **Необходимые компоненты**  
- **Docker** ([Инструкция по установке](https://docs.docker.com/engine/install/))  

### **🚀 Быстрый старт с Docker**  

```bash
git clone git@github.com:Robosoft-MIK-2025/star-lander.git
cd star-lander
docker compose up --build <TO-DO>
```

---

## 🛠 **Руководство по разработке**  

### **1. Локальная разработка (без Docker)**  
*TO-DO*  

### **2. Разработка в Docker**  

Скачайте репозиторий и перейдидте в его корень
```bash
git clone git@github.com:Robosoft-MIK-2025/star-lander.git
cd star-lander
```

В корне репозитория запустите
```
git clone https://github.com/PX4/PX4-Autopilot.git --recursive
```

Разрешите Docker использовать GUI приложения
```bash
nano ~/.bashrc
```

в конец файла добавить
```bash
# docker gui enable
xhost +local:docker
```

Запустите контейнер: 
> [!TIP] 
> перейти в директорию с Dockerfile и docker-compose.yaml
> (что-то типо: your-user@your-pc:..../star-lender$)
```bash
docker compose up --build terminal
```

Подключитесь к контейнеру из других терминалов:  
```bash
docker compose exec terminal bash
# Теперь мы в докер контейнере
```

Сборка пакетов этого репозитория:
```bash
cd /root/ros2_px4_ws
colcon build --packages-select tf_pkg apriltag_pkg bringup_pkg camera_pkg rviz_pkg
. install/setup.bash
```

Запускаем агент для связи между px4 и ros2:
```bash
MicroXRCEAgent udp4 -p 8888
```


Подключитесь к контейнеру из других терминалов:
> [!TIP] 
> перейти в директорию с Dockerfile и docker-compose.yaml
> (что-то типо: your-user@your-pc:..../star-lender$)
```bash
docker compose exec terminal bash
# Теперь мы в докер контейнере
```
Сборка репозитория PX4 и запуск тестового дрона
(чтобы проверить что всё работает)
```bash
cd src/PX4-Autopilot/
make px4_sitl gz_x500_vision
```

Подключитесь к контейнеру из других терминалов:  
```bash
docker compose exec terminal bash
# Теперь мы в докер контейнере
```

Запуск QGC для отслеживания местоположения дрона 
(и удобного вызова базовых команд таких как
- взлёт,
- посадка,
- перемещение дрона
- и т д)
```bash
su mobile
APPIMAGE_EXTRACT_AND_RUN=1 ./QGroundControl-x86_64.AppImage
```

Подключитесь к контейнеру из других терминалов:  
```bash
docker compose exec terminal bash
# Теперь мы в докер контейнере
```

Запуск всех пакетов *_pkg этого репозитория
```bash
ros2 launch bringup_pkg bringup.launch.py
```
### **3. Сборка Docker**  

После внесения изменений в Dockerfile необходимо собрать новый образ и загрузить его в облако (опционально).  

Сборка образа:  
```bash
docker build -t image_name -f Dockerfile .
```

Тег образа:  
```bash
docker tag image_name fabook/mik:common
```

Загрузка образа в Docker Hub:  
```bash
docker push fabook/mik:common
```

> [!IMPORTANT]  
> Для обеспечения воспроизводимости необходимо поддерживать актуальную версию Docker-образа в Docker Hub.  
> Однако не обязательно выполнять эту команду при каждой сборке, только когда вы уверены в своих изменениях

> [!TIP]  
> Тег может быть любым, но по умолчанию `docker compose` использует тег `latest`.  

---

## **5. Правила работы с Git и коммитами**  

### 🔹 **Основные теги (типы коммитов)**  
| Тег         | Описание                                                                 |
|-------------|--------------------------------------------------------------------------|
| **feat**    | Новая функциия. Пример: `feat: добавлена аутентификация`        |
| **fix**     | Исправление ошибки. Пример: `fix: исправлен краш при null-вводе`        |
| **docs**    | Изменения в документации. Пример: `docs: обновлен README.md`            |
| **style**   | Изменения форматирования (пробелы, запятые). Пример: `style: форматирование по PEP8` |
| **refactor**| Рефакторинг кода (без изменения функционала). Пример: `refactor: оптимизация функции X` |
| **perf**    | Улучшение производительности. Пример: `perf: уменьшено время загрузки` |
| **test**    | Изменения, связанные с тестами. Пример: `test: добавлено покрытие API` |
| **chore**   | Технические задачи (зависимости, конфиги). Пример: `chore: обновление webpack` |
| **ci**      | Изменения CI/CD (GitHub Actions, GitLab CI). Пример: `ci: добавлен деплой на staging` |
| **build**   | Изменения в системе сборки. Пример: `build: добавлен Dockerfile`       |
| **revert**  | Отмена предыдущего коммита. Пример: `revert: отмена коммита 123abc`    |

### 🔹 **Дополнительные правила**  
1. **Сообщение** должно быть четким и лаконичным.  
   - ❌ Плохо: `fix: баг`  
   - ✅ Хорошо: `fix: исправлена ошибка отправки формы`  

2. **Тело коммита** (опционально) — подробное описание изменений.  
   ```  
   fix: устранена утечка памяти в модуле X  

   Утечка возникала из-за незакрытых соединений с БД при долгих сессиях.  
   Добавлен `cleanup()` для корректного освобождения ресурсов.  
   ```  

3. **Футер** (опционально) — ссылки на задачи, критические изменения.  
   ```  
   feat: добавлена поддержка WebSocket  

   BREAKING CHANGE: Устаревший API `/chat` больше не поддерживается.  
   Closes #123  
   ```  

---

## 📌 **Как загрузить изменения в отдельную ветку**  

```bash
# 1. Создать новую ветку  
git checkout -b <имя_ветки>  
```

# 2. Проверить текущую ветку  
```bash
git branch  
```

# 3. Добавить изменения  
```bash
git add .  
```

# 4. Создать коммит  
```bash
git commit -m "<тип>: <описание>"  
```

# 5. Загрузить ветку на GitHub  
```bash
git push -u origin <имя_ветки>  
```
