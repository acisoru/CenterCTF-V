<template>
    <div class="not-found-container">
        <h1>404 - Страница Не Найдена</h1>
        <p>Увы, запрошенная страница не существует.</p>
        <router-link to="/">Вернуться на Главную</router-link>
    </div>
</template>

<script setup>

</script>

<style scoped>
.not-found-container {
    text-align: center;
    padding: 50px;
     font-family: sans-serif;
}
h1 {
    color: #dc3545;
    margin-bottom: 20px;
}
p {
    color: #666;
    margin-bottom: 30px;
}
a {
    padding: 10px 20px;
    background-color: #007bff;
    color: white;
    text-decoration: none;
    border-radius: 5px;
}
a:hover {
    background-color: #0056b3;
}
</style>