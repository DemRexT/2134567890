const express = require("express");
const path = require("path");
const session = require("express-session");
const multer = require("multer");
const bcrypt = require("bcryptjs");
const { Pool } = require("pg");
const fs = require("fs/promises");
require("dotenv").config({ path: path.join(__dirname, ".env") });

const app = express();
const upload = multer({ storage: multer.memoryStorage(), limits: { fileSize: 8 * 1024 * 1024 } });
const port = Number(process.env.PORT || 4173);

const pool = new Pool({
  connectionString: process.env.DATABASE_URL,
});

const sessionSecret = process.env.SESSION_SECRET;
if (!sessionSecret) {
  throw new Error("SESSION_SECRET is required. See album/.env.example");
}

app.use(express.json());
app.use(
  session({
    secret: sessionSecret,
    resave: false,
    saveUninitialized: false,
    cookie: {
      httpOnly: true,
      sameSite: "lax",
      secure: false,
      maxAge: 1000 * 60 * 60 * 12,
    },
  }),
);

function requireAuth(req, res, next) {
  if (!req.session.userId) {
    return res.status(401).json({ error: "Нужна авторизация" });
  }

  return next();
}

app.get("/api/session", (req, res) => {
  if (!req.session.userId) {
    return res.json({ authenticated: false });
  }

  return res.json({ authenticated: true, username: req.session.username });
});

app.post("/api/login", async (req, res) => {
  const { username, password } = req.body || {};
  if (!username || !password) {
    return res.status(400).json({ error: "Введите логин и пароль" });
  }

  const query = await pool.query("SELECT id, username, password_hash FROM users WHERE username = $1", [username]);
  const user = query.rows[0];

  if (!user) {
    return res.status(401).json({ error: "Неверный логин или пароль" });
  }

  const ok = await bcrypt.compare(password, user.password_hash);
  if (!ok) {
    return res.status(401).json({ error: "Неверный логин или пароль" });
  }

  req.session.userId = user.id;
  req.session.username = user.username;
  return res.json({ ok: true, username: user.username });
});

app.post("/api/logout", (req, res) => {
  req.session.destroy(() => {
    res.clearCookie("connect.sid");
    res.json({ ok: true });
  });
});

app.get("/api/photos", requireAuth, async (req, res) => {
  const query = await pool.query(
    "SELECT id, filename, mime_type, image_data, created_at FROM photos WHERE user_id = $1 ORDER BY created_at DESC",
    [req.session.userId],
  );

  res.json(
    query.rows.map((row) => ({
      id: row.id,
      filename: row.filename,
      mimeType: row.mime_type,
      src: row.image_data,
      createdAt: row.created_at,
    })),
  );
});

app.post("/api/photos", requireAuth, upload.array("photos", 20), async (req, res) => {
  const files = req.files || [];

  if (!files.length) {
    return res.status(400).json({ error: "Файлы не переданы" });
  }

  const client = await pool.connect();
  try {
    await client.query("BEGIN");

    for (const file of files) {
      if (!file.mimetype.startsWith("image/")) {
        throw new Error("Можно загружать только изображения");
      }

      const dataUrl = `data:${file.mimetype};base64,${file.buffer.toString("base64")}`;
      await client.query(
        "INSERT INTO photos (user_id, filename, mime_type, image_data) VALUES ($1, $2, $3, $4)",
        [req.session.userId, file.originalname, file.mimetype, dataUrl],
      );
    }

    await client.query("COMMIT");
    return res.json({ ok: true, uploaded: files.length });
  } catch (error) {
    await client.query("ROLLBACK");
    return res.status(400).json({ error: error.message || "Ошибка загрузки" });
  } finally {
    client.release();
  }
});

app.delete("/api/photos", requireAuth, async (req, res) => {
  await pool.query("DELETE FROM photos WHERE user_id = $1", [req.session.userId]);
  res.json({ ok: true });
});

app.use(express.static(__dirname));

app.get("*", (_, res) => {
  res.sendFile(path.join(__dirname, "index.html"));
});

async function bootstrap() {
  const schema = await fs.readFile(path.join(__dirname, "db.sql"), "utf-8");
  await pool.query(schema);

  const username = process.env.AUTH_USER || "admin";
  const password = process.env.AUTH_PASSWORD || "album123";

  const existing = await pool.query("SELECT id FROM users WHERE username = $1", [username]);
  if (!existing.rows[0]) {
    const hash = await bcrypt.hash(password, 12);
    await pool.query("INSERT INTO users (username, password_hash) VALUES ($1, $2)", [username, hash]);
    console.log(`Created default user: ${username}`);
  }

  app.listen(port, () => {
    console.log(`Album app running at http://localhost:${port}`);
  });
}

bootstrap().catch((error) => {
  console.error("Failed to start app:", error);
  process.exit(1);
});
