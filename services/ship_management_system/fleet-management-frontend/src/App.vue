<template>
  <div id="app-container">
    <header class="app-header">
      <nav>
        <router-link to="/" class="brand">Флот Петра I</router-link>
        <div class="nav-links">
          <template v-if="authStore.isAuthenticated">
             <router-link v-if="isUser" to="/dashboard">Обзор</router-link>
             <router-link v-if="isAdmiral" to="/admiral">Панель Адмирала</router-link>
             <router-link v-if="isCaptain" to="/captain">Панель Капитана</router-link>
            <span> | Пользователь: {{ authStore.userName }} ({{ displayRole }})</span>
            <button @click="logout" class="logout-button">Выйти</button>
          </template>
          <template v-else>
            <router-link to="/login">Вход</router-link>
            <router-link to="/register">Регистрация</router-link>
          </template>
        </div>
      </nav>
    </header>

    <main>
      <RouterView />
    </main>

    <footer class="app-footer">
       © {{ new Date().getFullYear() }} Управление Флотом Петра Великого.
    </footer>
  </div>
</template>

<script setup>
import { computed, onMounted } from 'vue';
import { RouterLink, RouterView, useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth';

const authStore = useAuthStore();
const router = useRouter();

const isUser = computed(() => authStore.isAuthenticated && (authStore.userRole === 'Sailor' || !authStore.userRole));
const isAdmiral = computed(() => authStore.isAuthenticated && authStore.userRole === 'Admiral');
const isCaptain = computed(() => authStore.isAuthenticated && authStore.userRole === 'Captain');

const displayRole = computed(() => {
    switch(authStore.userRole) {
        case 'Admiral': return 'Адмирал';
        case 'Captain': return 'Капитан';
        case 'Sailor': return 'Матрос';
        case '':
        case null:
        case undefined:
             return 'Без роли';
        default: return authStore.userRole;
    }
});

const logout = () => {
  authStore.logout();
};

onMounted(() => {
    authStore.checkAuth();
});

</script>

<style>
html, body {
  height: 100%;
  margin: 0;
  padding: 0;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background-color: #e8f0f5;
  color: #333;
}

#app-container {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
  width: 100%;
  max-width: 1400px;
  margin-left: auto;
  margin-right: auto;
  background-color: #ffffff;
  box-shadow: 0 0 25px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
}

main {
  flex-grow: 1;
  padding: 20px;
  width: 100%;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
}

.app-header {
  background-color: #1a237e;
  color: white;
  padding: 15px 30px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
  flex-shrink: 0;
  box-sizing: border-box;
}

.app-header nav {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.brand {
  font-size: 1.5em;
  font-weight: bold;
  color: white;
  text-decoration: none;
}

.nav-links {
  display: flex;
  align-items: center;
  gap: 20px;
}

.nav-links a {
  color: #e0e0e0;
  text-decoration: none;
  font-size: 1em;
  transition: color 0.3s;
}

.nav-links a:hover,
.nav-links a.router-link-exact-active {
  color: #ffffff;
  font-weight: bold;
}

.nav-links span {
    color: #bdbdbd;
    font-size: 0.9em;
}

.logout-button {
  background-color: #c62828;
  color: white;
  border: none;
  padding: 8px 15px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 0.9em;
  transition: background-color 0.3s;
}

.logout-button:hover {
  background-color: #b71c1c;
}

.app-footer {
    background-color: #37474f;
    color: #cfd8dc;
    text-align: center;
    padding: 15px;
    font-size: 0.9em;
    flex-shrink: 0;
    box-sizing: border-box;
}

.dashboard-container {
    width: 100%;
    max-width: 1100px;
    margin: 20px auto;
    padding: 20px;
    border-radius: 8px;
    box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    background-color: #fff;
}
.admiral-dashboard { background-color: #f8f0f0; }
.captain-dashboard { background-color: #f0f8f0; }
.user-dashboard { background-color: #f0f4f8; }

.loading { color: #555; font-style: italic; text-align: center; padding: 15px; }
.error-message { color: #721c24; background-color: #f8d7da; border: 1px solid #f5c6cb; padding: 10px; border-radius: 4px; margin-top: 15px; text-align: center; }
.success-message { color: #155724; background-color: #d4edda; border: 1px solid #c3e6cb; padding: 10px; border-radius: 4px; margin-top: 15px; text-align: center; }
.no-data { color: #666; font-style: italic; text-align: center; padding: 15px; }

</style>