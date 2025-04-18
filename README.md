
# Multithreaded Portable File Sharing Program (C, C++) 

**Version:** Alpha 0.1.5

## Обзор

Multithreaded Portable File Sharing Program – это кроссплатформенное приложение, написанное на C/C++, которое обеспечивает обмен файлами через сеть с использованием многопоточности. Сервер способен одновременно обрабатывать несколько подключений клиентов, предоставляя функциональность для работы с файловой системой, такую как просмотр содержимого каталогов, смена директории, загрузка файлов, получение статистики и поиск по шаблону.

## Особенности

- **Многопоточность:** Обработка нескольких клиентских подключений с использованием POSIX threads (или их эквивалента в Windows).
- **Кроссплатформенность:** Совместимость с Windows и UNIX-подобными системами.
- **Управление файлами:** Поддержка команд для просмотра файлов, смены каталогов, загрузки файлов и получения информации о файлах.
- **Логирование:** Запись активности клиентов с указанием временных меток для удобной отладки и аудита.
- **Простота использования:** Интуитивно понятный интерфейс для клиентов с поддержкой текстовых команд.

## Структура проекта

```
Multithreaded-portable-file-sharing-program-c-c-
├── include/
│   ├── file_manager.h          # Заголовочный файл с функционалом работы с файлами
│   ├── sendStandart.h          # Заголовочный файл для отправки данных
├── src/
│   ├── main_server.cpp         # Код сервера
│   ├── main_client.cpp         # Код клиента
│   ├── file_manager.cpp        # Заголовочный файл с функционалом работы с файлами
│   ├── file_utils.cpp        
├── README.md                   # Этот файл
```

## Требования

- **Компилятор:** Поддержка C++17 или выше (для работы с файловой системой и потоками)
- **Библиотеки:** 
  - POSIX threads (на Linux/Unix)
  - Winsock2 (на Windows)
- **ОС:** Windows, Linux, macOS или другие UNIX-подобные системы
- **Make:** Установленная утилита make для сборки проекта через Makefile

## Инструкции по сборке

### 1. Клонирование репозитория

Склонируйте репозиторий с GitHub:

```bash
git clone https://github.com/Photographer995/Multithreaded-portable-file-sharing-program-c-c-.git
cd Multithreaded-portable-file-sharing-program-c-c-
```

### 2. Сборка проекта с использованием Makefile

Для сборки проекта выполните команду:

```bash
make
```

### 3. Запуск приложения

- **Сервер:**

  Запустите сервер, указав порт и корневой каталог для работы:

  ```bash
  ./server <порт> <путь_к_корневому_каталогу>
  ```

- **Клиент:**

  Подключитесь к серверу, указав IP-адрес и порт:

  ```bash
  ./client <IP_сервера> <порт>
  ```

## Использование

После подключения клиента к серверу доступны следующие команды:

- **ls**  
  Отображает список файлов и каталогов на сервере.

- **cd `<каталог>`**  
  Изменяет текущий каталог.  
  Пример: `cd documents`

- **get `<файл>` [offset]**  
  Загружает файл с сервера. Дополнительно можно указать смещение (offset) для продолжения загрузки.  
  Пример: `get example.txt` или `get example.txt 1024`

- **stat `<файл>`**  
  Показывает информацию о файле или каталоге.

- **find [опция] `<шаблон>`**  
  Ищет файлы и каталоги по указанному шаблону.  
  - `-f` – поиск только файлов  
  - `-d` – поиск только каталогов  
  - Без опций – поиск и файлов, и каталогов  
  Пример: `find -f report`

- **help**  
  Выводит справочную информацию о доступных командах.

- **exit**  
  Завершает соединение с сервером.

## Логирование

Сервер ведёт логирование действий клиентов, включая подключения, команды и ошибки, с указанием временных меток. Это облегчает отладку и мониторинг работы сервера.

## Версия

**Alpha 0.1.5:**  
Данный релиз находится на стадии альфа-тестирования. Ожидается внесение улучшений, исправлений ошибок и добавление новых функций в будущих версиях.


## Вклад и обратная связь

Мы рады вашим предложениям и улучшениям!  
Если у вас есть идеи, исправления или вы обнаружили ошибки, пожалуйста, создайте issue или отправьте pull request в репозиторий.

## Контакты

Для получения дополнительной информации или поддержки обращайтесь:  
[Photographer995](mailto:svyatoslav1995g@gmail.com)

---

*Проект предоставляется "как есть", без каких-либо гарантий. Используйте на свой страх и риск.*
```

---
