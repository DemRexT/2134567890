# Защищённый фотоальбом

## Что сделано
- Авторизация через форму логина/пароля.
- Все операции с фото доступны только после входа.
- Фото хранятся в PostgreSQL (таблица `photos`), а не в `localStorage`.
- При первом запуске автоматически создаётся пользователь из переменных окружения `AUTH_USER`/`AUTH_PASSWORD`.

## Быстрый старт
1. Скопируйте окружение:
   ```bash
   cp album/.env.example album/.env
   ```
2. Создайте базу данных PostgreSQL (например, `album_db`) и проверьте `DATABASE_URL`.
3. Установите зависимости:
   ```bash
   cd album
   npm install
   ```
4. Запустите сервер:
   ```bash
   npm start
   ```
5. Откройте в браузере `http://localhost:4173`.

## API
- `POST /api/login` — вход.
- `POST /api/logout` — выход.
- `GET /api/session` — статус сессии.
- `GET /api/photos` — получить фото текущего пользователя.
- `POST /api/photos` — загрузка фото (multipart поле `photos`).
- `DELETE /api/photos` — удалить все фото текущего пользователя.
