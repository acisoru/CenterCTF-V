<template>
  <div class="auth-view-wrapper">
    <div class="auth-container">
      <h2>Регистрация Нового Пользователя</h2>
      <form @submit.prevent="handleRegister">
        <div class="form-group">
          <label for="firstname">Имя:</label>
          <input type="text" id="firstname" v-model="userData.firstname" required>
        </div>
        <div class="form-group">
          <label for="secondname">Отчество:</label>
          <input type="text" id="secondname" v-model="userData.secondname" required>
        </div>
        <div class="form-group">
          <label for="thirdname">Фамилия:</label>
          <input type="text" id="thirdname" v-model="userData.thirdname" required>
        </div>
        <div class="form-group">
          <label for="password">Пароль:</label>
          <input type="password" id="password" v-model="userData.password" required>
        </div>
        <div class="form-group">
          <label for="confirmPassword">Подтвердите Пароль:</label>
          <input type="password" id="confirmPassword" v-model="confirmPassword" required>
        </div>
        <p v-if="passwordMismatch" class="error-message">Пароли не совпадают!</p>
        <button type="submit" :disabled="authStore.status === 'loading' || passwordMismatch">
          {{ authStore.status === 'loading' ? 'Регистрация...' : 'Зарегистрироваться' }}
        </button>
        <p v-if="authStore.error && authStore.status === 'error'" class="error-message">{{ authStore.error }}</p>
      </form>
      <p class="switch-link">
        Уже есть аккаунт? <router-link to="/login">Войти</router-link>
      </p>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue';
import { useAuthStore } from '@/stores/auth';

const authStore = useAuthStore();
const userData = ref({
  firstname: '',
  secondname: '',
  thirdname: '',
  password: ''
});
const confirmPassword = ref('');

const passwordMismatch = computed(() => {
    return userData.value.password && confirmPassword.value && userData.value.password !== confirmPassword.value;
});

const handleRegister = () => {
  if (passwordMismatch.value) {
      return;
  }
  authStore.register(userData.value);
};
</script>

<style scoped>
.auth-view-wrapper {
  display: flex;
  justify-content: center;
  align-items: center;
  flex-grow: 1;
  width: 100%;
  padding: 20px;
  box-sizing: border-box;
}

.auth-container {
  width: 100%;
  max-width: 550px;
  padding: 40px;
  border: 1px solid #d1d9e6;
  border-radius: 10px;
  background-color: #f9f9f9;
  box-shadow: 0 6px 15px rgba(0, 0, 0, 0.1);
  font-family: sans-serif;
}

h2 {
  text-align: center;
  margin-bottom: 30px;
  color: #333;
  font-size: 1.6em;
}

.form-group {
  margin-bottom: 25px;
}

label {
  display: block;
  margin-bottom: 8px;
  font-weight: bold;
  color: #555;
  font-size: 0.95em;
}

input[type="text"],
input[type="password"] {
  width: 100%;
  padding: 12px;
  border: 1px solid #ccc;
  border-radius: 5px;
  box-sizing: border-box;
  font-size: 1em;
}

button {
  display: block;
  margin-left: auto;
  margin-right: auto;
  min-width: 160px;
  padding: 12px 30px;
  background-color: #28a745;
  color: white;
  border: none;
  border-radius: 5px;
  cursor: pointer;
  font-size: 1.05em;
  transition: background-color 0.3s ease;
  margin-top: 5px;
}

button:disabled {
  background-color: #aaa;
  cursor: not-allowed;
}

button:not(:disabled):hover {
  background-color: #218838;
}

.error-message {
  color: #d9534f;
  margin-top: 15px;
  margin-bottom: 10px;
  text-align: center;
  font-weight: bold;
}

.switch-link {
  text-align: center;
  margin-top: 25px;
  font-size: 0.95em;
}

.switch-link a {
  color: #0056b3;
  text-decoration: none;
  font-weight: bold;
}

.switch-link a:hover {
  text-decoration: underline;
}
</style>