const PHOTOS_PER_PAGE = 4;

const authSection = document.getElementById("authSection");
const albumSection = document.getElementById("albumSection");
const authMessage = document.getElementById("authMessage");
const currentUser = document.getElementById("currentUser");
const loginForm = document.getElementById("loginForm");

const bookEl = document.getElementById("book");
const pageIndicator = document.getElementById("pageIndicator");
const photoInput = document.getElementById("photoInput");
const prevBtn = document.getElementById("prevBtn");
const nextBtn = document.getElementById("nextBtn");
const clearBtn = document.getElementById("clearBtn");
const logoutBtn = document.getElementById("logoutBtn");
const pageTemplate = document.getElementById("pageTemplate");

let photos = [];
let currentPageIndex = 0;

init();

async function init() {
  loginForm.addEventListener("submit", onLogin);
  logoutBtn.addEventListener("click", onLogout);

  photoInput.addEventListener("change", onUpload);
  prevBtn.addEventListener("click", () => {
    currentPageIndex = Math.max(0, currentPageIndex - 1);
    updateFlipState();
  });

  nextBtn.addEventListener("click", () => {
    const maxPage = Math.max(0, Math.ceil(photos.length / PHOTOS_PER_PAGE) - 1);
    currentPageIndex = Math.min(maxPage, currentPageIndex + 1);
    updateFlipState();
  });

  clearBtn.addEventListener("click", clearPhotos);

  await checkSession();
}

async function apiRequest(url, options) {
  const response = await fetch(url, options);
  const contentType = response.headers.get("content-type") || "";
  const payload = contentType.includes("application/json") ? await response.json() : null;
  return { response, payload };
}

async function checkSession() {
  try {
    const { payload } = await apiRequest("/api/session");

    if (!payload?.authenticated) {
      showAuth();
      return;
    }

    currentUser.textContent = payload.username;
    showAlbum();
    await loadPhotos();
  } catch {
    showAuth("Сервер недоступен. Запусти backend из папки album.");
  }
}

async function onLogin(event) {
  event.preventDefault();
  authMessage.textContent = "";

  const formData = new FormData(loginForm);
  const username = String(formData.get("username") || "").trim();
  const password = String(formData.get("password") || "");

  try {
    const { response, payload } = await apiRequest("/api/login", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password }),
    });

    if (!response.ok) {
      authMessage.textContent = payload?.error || "Ошибка авторизации";
      return;
    }

    currentUser.textContent = payload.username;
    showAlbum();
    await loadPhotos();
    loginForm.reset();
  } catch {
    authMessage.textContent = "Не удалось связаться с сервером";
  }
}

async function onLogout() {
  await fetch("/api/logout", { method: "POST" });
  showAuth();
}

async function onUpload(event) {
  const files = Array.from(event.target.files || []);
  if (!files.length) {
    return;
  }

  const formData = new FormData();
  files.forEach((file) => formData.append("photos", file));

  const { response, payload } = await apiRequest("/api/photos", {
    method: "POST",
    body: formData,
  });

  if (!response.ok) {
    alert(payload?.error || "Не удалось загрузить фото");
    return;
  }

  await loadPhotos(true);
  photoInput.value = "";
}

async function loadPhotos(jumpToLast = false) {
  const { response, payload } = await apiRequest("/api/photos");
  if (response.status === 401) {
    showAuth();
    return;
  }

  photos = Array.isArray(payload) ? payload : [];

  if (jumpToLast && photos.length) {
    currentPageIndex = Math.max(0, Math.ceil(photos.length / PHOTOS_PER_PAGE) - 1);
  }

  renderBook();
}

async function clearPhotos() {
  if (!window.confirm("Удалить все фото из альбома?")) {
    return;
  }

  await fetch("/api/photos", { method: "DELETE" });
  photos = [];
  currentPageIndex = 0;
  renderBook();
}

function showAuth(message = "") {
  authSection.classList.remove("hidden");
  albumSection.classList.add("hidden");
  authMessage.textContent = message;
}

function showAlbum() {
  authSection.classList.add("hidden");
  albumSection.classList.remove("hidden");
}

function renderBook() {
  bookEl.innerHTML = "";

  if (!photos.length) {
    const empty = document.createElement("div");
    empty.className = "empty-state";
    empty.innerHTML = "<div><strong>Альбом пуст</strong><br>Нажми «Добавить фото», чтобы начать.</div>";
    bookEl.appendChild(empty);
    pageIndicator.textContent = "Страница 1 / 1";
    prevBtn.disabled = true;
    nextBtn.disabled = true;
    return;
  }

  const pages = chunkPhotos(photos, PHOTOS_PER_PAGE);

  pages.forEach((pagePhotos, index) => {
    const page = pageTemplate.content.firstElementChild.cloneNode(true);
    const front = page.querySelector(".page__content");

    pagePhotos.forEach((photo) => {
      const card = document.createElement("figure");
      card.className = "photo-card";

      const img = document.createElement("img");
      img.src = photo.src;
      img.alt = photo.filename || "Фото в альбоме";

      card.appendChild(img);
      front.appendChild(card);
    });

    const back = document.createElement("div");
    back.className = "page__back";
    back.textContent = `Страница ${index + 1}`;
    page.appendChild(back);

    page.style.zIndex = String(pages.length - index);
    page.dataset.index = String(index);

    bookEl.appendChild(page);
  });

  currentPageIndex = Math.min(currentPageIndex, pages.length - 1);
  updateFlipState();
}

function updateFlipState() {
  const pages = Array.from(bookEl.querySelectorAll(".page"));
  const pageCount = pages.length || 1;

  pages.forEach((page) => {
    const index = Number(page.dataset.index || "0");
    page.classList.toggle("flipped", index < currentPageIndex);
  });

  pageIndicator.textContent = `Страница ${Math.min(currentPageIndex + 1, pageCount)} / ${pageCount}`;

  prevBtn.disabled = currentPageIndex === 0;
  nextBtn.disabled = currentPageIndex >= pageCount - 1;
}

function chunkPhotos(items, size) {
  const chunks = [];

  for (let i = 0; i < items.length; i += size) {
    chunks.push(items.slice(i, i + size));
  }

  return chunks;
}
