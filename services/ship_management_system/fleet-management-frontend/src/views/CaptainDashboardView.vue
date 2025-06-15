<template>
    <div class="dashboard-container captain-dashboard">
      <h1>Панель Управления Капитана</h1>
      <p>Добро пожаловать, Капитан {{ authStore.userName }}!</p>
  
      <div class="ship-info-section">
          <div v-if="loadingShipInfo" class="loading">Загрузка корабля...</div>
          <div v-else-if="shipError" class="error-message">{{ shipError }}</div>
          <p v-else class="ship-info-display">
              Ваш Корабль:
              <strong v-if="shipInfo.id">{{ shipInfo.name }} (ID: {{ shipInfo.id }})</strong>
              <span v-else>Не назначен</span>
          </p>
      </div>
  
  
      <section class="mission-management-section">
          <h2>Ваша Текущая Миссия</h2>
          <button @click="fetchCurrentMission" :disabled="loadingMission" class="refresh-button section-refresh-button">
              {{ loadingMission ? 'Проверка...' : 'Проверить Миссию' }}
          </button>
  
          <div v-if="loadingMission" class="loading">Загрузка данных о миссии...</div>
          <div v-else-if="missionError" class="error-message">{{ missionError }}</div>
          <div v-else-if="mission && mission.mission_id !== null" class="mission-details-container">
              <div class="mission-details">
                   <p><strong>Статус:</strong> <span :class="mission.isComplete ? 'status-completed' : 'status-active'">{{ mission.isComplete ? 'Завершена' : 'Активна' }}</span></p>
                   <p class="mission-description-text"><strong>Описание:</strong> {{ mission.description }}</p>
              </div>
               <button
                  v-if="!mission.isComplete"
                  @click="handleMarkComplete"
                  :disabled="markingComplete"
                  class="action-button mark-complete-button"
               >
                  {{ markingComplete ? 'Отметка...' : 'Отметить как Завершенную' }}
              </button>
              <p v-if="markCompleteError" class="error-message">{{ markCompleteError }}</p>
              <p v-if="markCompleteSuccess" class="success-message">{{ markCompleteSuccess }}</p>
          </div>
          <div v-else-if="!missionError" class="no-data">В данный момент вам не назначена миссия.</div>
      </section>
  
      <section class="requests-section">
        <h2>Прошения о Зачислении в Матросы</h2>
         <button @click="fetchSailorRequests" :disabled="loadingRequests" class="refresh-button section-refresh-button">
           {{ loadingRequests ? 'Обновление...' : 'Обновить Список Прошений' }}
        </button>
        <div v-if="loadingRequests" class="loading">Загрузка прошений...</div>
        <div v-else-if="requestsError" class="error-message">{{ requestsError }}</div>
        <ul v-else-if="sailorRequests.length > 0" class="request-list">
          <li v-for="request in sailorRequests" :key="request.id" class="request-item">
            <span>Прошение от Пользователя ID: <strong>{{ request.asker_id }}</strong></span>
            <div class="request-actions">
              <button @click="handleAcceptSailor(request.asker_id)" class="accept" :disabled="processingRequest === request.asker_id">
                 {{ processingRequest === request.asker_id ? 'Принятие...' : 'Принять в Матросы' }}
              </button>
              <button @click="handleDeclineSailor(request.asker_id)" class="decline" :disabled="processingRequest === request.asker_id">
                 {{ processingRequest === request.asker_id ? 'Отклонение...' : 'Отклонить' }}
              </button>
            </div>
          </li>
        </ul>
        <div v-else class="no-data">Нет ожидающих прошений на должность матроса.</div>
         <p v-if="requestActionError" class="error-message">{{ requestActionError }}</p>
         <p v-if="requestActionSuccess" class="success-message">{{ requestActionSuccess }}</p>
      </section>
  
        <section class="crew-management-section">
          <h2>Экипаж Корабля</h2>
  
          <div class="list-container crew-list-container">
               <button @click="fetchSailors" :disabled="loadingSailors" class="refresh-button list-refresh-button">
                  {{ loadingSailors ? 'Обновление...' : 'Обновить Список Экипажа' }}
              </button>
              <div v-if="loadingSailors" class="loading">Загрузка экипажа...</div>
              <div v-else-if="sailorsError" class="error-message">{{ sailorsError }}</div>
              <ul v-else-if="sailors.length > 0" class="data-list sailor-list">
                   <li v-for="sailor in sailors" :key="sailor.id" class="data-item sailor-item">
                      <div class="sailor-info">
                          <span><strong>{{ sailor.fullname }}</strong> (ID: {{ sailor.id }})</span>
                           <div v-if="editingSailorId === sailor.id" class="edit-description-active">
                               <textarea
                                  v-model="editingSailorDescription"
                                  placeholder="Введите новое описание матроса..."
                                  rows="3"
                               ></textarea>
                               <div class="edit-actions">
                                  <button @click="saveSailorDescription(sailor.id)" :disabled="savingSailorDescription" class="action-button save-button">
                                      {{ savingSailorDescription ? 'Сохр...' : 'Сохранить Описание' }}
                                  </button>
                                  <button @click="cancelEditingSailorDescription" class="action-button cancel-button">Отмена</button>
                               </div>
                               <p v-if="changeSailorDescriptionError && editingSailorId === sailor.id" class="error-message inline-error">{{ changeSailorDescriptionError }}</p>
                               <p v-if="changeSailorDescriptionSuccess && editingSailorId === sailor.id" class="success-message inline-success">{{ changeSailorDescriptionSuccess }}</p>
                           </div>
                      </div>
                       <div class="sailor-actions">
                           <button
                              @click="startEditingSailorDescription(sailor.id)"
                              v-if="editingSailorId !== sailor.id"
                              class="action-button edit-desc-button"
                              title="Редактировать описание">
                              Ред. Описание
                          </button>
                       </div>
                   </li>
              </ul>
               <div v-else class="no-data">На вашем корабле нет экипажа.</div>
          </div>
       </section>
    </div>
  </template>
  
  <script setup>
  import { ref, onMounted } from 'vue';
  import apiClient from '@/services/api';
  import { useAuthStore } from '@/stores/auth';
  import { decode, encode } from '@/utils/base64';
  
  const authStore = useAuthStore();
  
  const shipInfo = ref({ name: null, id: null });
  const loadingShipInfo = ref(false);
  const shipError = ref(null);
  
  const sailorRequests = ref([]);
  const loadingRequests = ref(false);
  const requestsError = ref(null);
  const processingRequest = ref(null);
  const requestActionError = ref(null);
  const requestActionSuccess = ref(null);
  
  const mission = ref(null);
  const loadingMission = ref(false);
  const missionError = ref(null);
  const markingComplete = ref(false);
  const markCompleteError = ref(null);
  const markCompleteSuccess = ref(null);
  
  const sailors = ref([]);
  const loadingSailors = ref(false);
  const sailorsError = ref(null);
  const editingSailorId = ref(null);
  const editingSailorDescription = ref('');
  const savingSailorDescription = ref(false);
  const changeSailorDescriptionError = ref(null);
  const changeSailorDescriptionSuccess = ref(null);
  
  
  const fetchShipInfo = async () => {
       loadingShipInfo.value = true;
       shipError.value = null;
       shipInfo.value = { name: null, id: null };
       try {
           const response = await apiClient.get('/captain/my_ship');
           if (response.data && response.data.id && response.data.name) {
               shipInfo.value = {
                   id: response.data.id,
                   name: decode(response.data.name)
               };
           } else {
               console.log("Капитан не назначен на корабль:", decode(response.data?.message));
           }
       } catch (error) {
           console.error("Ошибка загрузки корабля:", error);
           if (error.response?.status === 404) {
              shipError.value = "Вы не назначены ни на один корабль.";
           } else if (error.response?.data?.message) {
               try { shipError.value = decode(error.response.data.message) || "Ошибка загрузки корабля."; } catch {}
           } else {
              shipError.value = "Не удалось загрузить информацию о корабле.";
           }
       } finally {
           loadingShipInfo.value = false;
       }
  };
  
  const fetchSailorRequests = async () => {
    loadingRequests.value = true; requestsError.value = null; requestActionError.value = null; requestActionSuccess.value = null; sailorRequests.value = [];
    try {
      const response = await apiClient.get('/captain/get_all_sailors_requests');
      if (response.data?.requests) { sailorRequests.value = response.data.requests; }
      else { requestsError.value = decode(response.data?.message) || "Не удалось загрузить прошения."; }
    } catch (error) { console.error("Err fetch sailor reqs:", error); requestsError.value = "Ошибка загрузки прошений."; if (error.response?.data?.message) { try { requestsError.value = decode(error.response.data.message) || requestsError.value; } catch {} } }
    finally { loadingRequests.value = false; }
  };
  const fetchCurrentMission = async () => {
      loadingMission.value = true; missionError.value = null; mission.value = null; markCompleteError.value = null; markCompleteSuccess.value = null;
      try {
          const response = await apiClient.get('/captain/check_mission');
          if (response.data && response.data.message) {
              const message = decode(response.data.message);
              if (response.data.mission_id !== undefined) {
                   mission.value = { mission_id: response.data.mission_id, description: decode(response.data.description), isComplete: !!response.data.isComplete };
              } else if (message.includes("No mission was assign")) { mission.value = null; }
              else { missionError.value = message || "Не удалось получить данные о миссии."; }
          } else { missionError.value = "Некорректный ответ от сервера."; }
      } catch (error) {
           console.error("Err fetch mission:", error); missionError.value = "Ошибка проверки миссии.";
           if (error.response?.data?.message) { try { missionError.value = decode(error.response.data.message) || missionError.value; } catch {} }
           else if (error.response?.data) { try { missionError.value = decode(String(error.response.data)) || missionError.value; } catch {} }
           if (error.response?.status === 404 && !missionError.value.includes("assign")) { missionError.value = "Невозможно проверить миссию. Вы не назначены на корабль?"; }
      } finally { loadingMission.value = false; }
  };
  const fetchSailors = async () => {
      loadingSailors.value = true; sailorsError.value = null; sailors.value = [];
      changeSailorDescriptionError.value = null; changeSailorDescriptionSuccess.value = null;
      try {
          const response = await apiClient.get('/captain/get_sailors_for_ship');
          if (response.data?.sailors) {
              sailors.value = response.data.sailors.map(s => ({
                  id: s.id,
                  fullname: [decode(s.firstname), decode(s.secondname), decode(s.thirdname)].filter(Boolean).join(' '),
              }));
          } else if (response.data?.message) { console.log('Fetch sailors:', decode(response.data.message)); }
      } catch (error) {
          console.error("Err fetch sailors:", error); sailorsError.value = "Ошибка загрузки экипажа.";
          if (error.response?.data?.message) { try { sailorsError.value = decode(error.response.data.message) || sailorsError.value; } catch {} }
          else if (error.response?.status === 404) { sailorsError.value = "Невозможно загрузить экипаж. Вы не назначены на корабль?"; }
      } finally { loadingSailors.value = false; }
  };
  
  
  const handleAcceptSailor = async (sailorId) => {
      processingRequest.value = sailorId; requestActionError.value = null; requestActionSuccess.value = null;
      try {
          const response = await apiClient.post('/captain/accept_sailor_request', { sailor_id: sailorId });
          requestActionSuccess.value = decode(response.data?.message) || 'Прошение одобрено.';
          await fetchSailorRequests(); await fetchSailors();
      } catch (error) { console.error("Err accept sailor:", error); requestActionError.value = "Ошибка одобрения."; if (error.response?.data?.message) { try { requestActionError.value = decode(error.response.data.message) || requestActionError.value; } catch {} } }
      finally { processingRequest.value = null; setTimeout(() => { requestActionSuccess.value = null; requestActionError.value = null; }, 4000); }
  };
  const handleDeclineSailor = async (sailorId) => {
      processingRequest.value = sailorId; requestActionError.value = null; requestActionSuccess.value = null;
      try {
          const response = await apiClient.post('/captain/decline_sailor_request', { sailor_id: sailorId });
          requestActionSuccess.value = decode(response.data?.message) || 'Прошение отклонено.';
          await fetchSailorRequests();
      } catch (error) { console.error("Err decline sailor:", error); requestActionError.value = "Ошибка отклонения."; if (error.response?.data?.message) { try { requestActionError.value = decode(error.response.data.message) || requestActionError.value; } catch {} } }
      finally { processingRequest.value = null; setTimeout(() => { requestActionSuccess.value = null; requestActionError.value = null; }, 4000); }
  };
  
  const handleMarkComplete = async () => {
      if (!mission.value || mission.value.isComplete) return;
      markingComplete.value = true; markCompleteError.value = null; markCompleteSuccess.value = null;
      try {
          const response = await apiClient.get('/captain/mark_mission_as_complete');
          markCompleteSuccess.value = decode(response.data?.message) || "Миссия завершена.";
          await fetchCurrentMission();
      } catch (error) {
           console.error("Err mark complete:", error); markCompleteError.value = "Ошибка завершения миссии.";
           if (error.response?.data?.message) { try { markCompleteError.value = decode(error.response.data.message) || markCompleteError.value; } catch {} }
           else if (error.response?.data) { try { markCompleteError.value = decode(String(error.response.data)) || markCompleteError.value; } catch {} }
      } finally { markingComplete.value = false; setTimeout(() => { markCompleteSuccess.value = null; markCompleteError.value = null; }, 4000); }
  };
  
  const startEditingSailorDescription = (sailorId) => {
      editingSailorDescription.value = '';
      editingSailorId.value = sailorId;
      changeSailorDescriptionError.value = null;
      changeSailorDescriptionSuccess.value = null;
  };
  const cancelEditingSailorDescription = () => {
      editingSailorId.value = null;
      editingSailorDescription.value = '';
  };
  const saveSailorDescription = async (sailorId) => {
      savingSailorDescription.value = true;
      changeSailorDescriptionError.value = null;
      changeSailorDescriptionSuccess.value = null;
      try {
          const payload = { sailor_id: sailorId, new_description: encode(editingSailorDescription.value) };
          const response = await apiClient.post('/captain/change_sailor_description', payload);
          changeSailorDescriptionSuccess.value = decode(response.data?.message) || "Описание матроса изменено.";
          editingSailorId.value = null;
      } catch (error) {
          console.error(`Err change sailor desc ${sailorId}:`, error);
          changeSailorDescriptionError.value = `Ошибка описания ID ${sailorId}.`;
           if (error.response?.data?.message) { try { changeSailorDescriptionError.value = decode(error.response.data.message) || changeSailorDescriptionError.value; } catch { } }
      } finally {
          savingSailorDescription.value = false;
      }
  };
  
  onMounted(() => {
    fetchShipInfo();
    fetchSailorRequests();
    fetchCurrentMission();
    fetchSailors();
  });
  </script>
  
  <style scoped>
  .dashboard-container { padding: 20px; font-family: sans-serif; max-width: 900px; margin: 20px auto; background-color: #f0f8f0; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
  h1, h2 { color: #006400; text-align: center; margin-bottom: 15px; }
  h2 { margin-top: 30px; border-bottom: 1px solid #ccc; padding-bottom: 10px; position: relative; }
  h3 { color: #2e8b57; margin-top: 20px; margin-bottom: 15px; text-align: center; font-size: 1.2em; }
  p { text-align: center; color: #333; margin-bottom: 20px; }
  section { margin-bottom: 40px; padding: 25px; background-color: #fff; border-radius: 6px; border: 1px solid #ddd; box-shadow: 0 2px 5px rgba(0,0,0,0.05);}
  button:disabled { opacity: 0.6; cursor: not-allowed; }
  
  .refresh-button { padding: 6px 12px; font-size: 0.9em; background-color: #6c757d; color: white; border: none; border-radius: 4px; cursor: pointer; transition: background-color 0.2s; }
  .refresh-button:hover:not(:disabled) { background-color: #5a6268; }
  .section-refresh-button { display: block; margin: 0 auto 20px auto; padding: 8px 15px; }
  .list-refresh-button { position: absolute; top: -35px; right: 0; }
  .action-button { padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 1em; transition: background-color 0.2s; }
  
  .list-container { position: relative; margin-top: 20px; padding-top: 10px; border-top: 1px solid #eee; }
  .data-list { list-style: none; padding: 0; margin-top: 10px; }
  .data-item { display: flex; justify-content: space-between; align-items: center; padding: 12px 15px; border: 1px solid #eee; border-radius: 4px; margin-bottom: 8px; background-color: #f9f9f9; }
  .data-item:nth-child(odd) { background-color: #fefefe; }
  
  
  .ship-info-section { text-align: center; margin-bottom: 25px; }
  .ship-info-display { font-size: 1.1em; margin-top: 5px; color: #555; display: inline-block; }
  .ship-info-display strong { color: #000; }
  .ship-info-display span { font-style: italic; color: #666; }
  
  .mission-management-section { border-top: 2px solid #006400; }
  .mission-management-section .section-refresh-button { background-color: #007bff; }
  .mission-management-section .section-refresh-button:hover:not(:disabled) { background-color: #0056b3; }
  .mission-details-container { background-color: #f8f9fa; border: 1px solid #dee2e6; border-left: 5px solid #006400; border-radius: 5px; padding: 20px; margin-top: 15px; }
  .mission-details p { text-align: left; margin-bottom: 12px; line-height: 1.5; color: #495057; }
  .mission-details p:last-child { margin-bottom: 0; }
  .mission-details strong { color: #343a40; min-width: 70px; display: inline-block; }
  .mission-description-text { white-space: pre-wrap; word-break: break-word; background-color: #e9ecef; padding: 10px; border-radius: 4px; }
  .status-active { color: #dc3545; font-weight: bold; background-color: #f8d7da; padding: 2px 6px; border-radius: 3px; }
  .status-completed { color: #28a745; font-weight: bold; background-color: #d4edda; padding: 2px 6px; border-radius: 3px; }
  .mark-complete-button { display: block; margin: 20px auto 0 auto; background-color: #28a745; color: white; }
  .mark-complete-button:hover:not(:disabled) { background-color: #218838; }
  
  .requests-section { border-top: 2px solid #006400; }
  .requests-section .section-refresh-button { background-color: #17a2b8; }
  .requests-section .section-refresh-button:hover:not(:disabled) { background-color: #138496; }
  .request-list { list-style: none; padding: 0; }
  .request-item { background-color: #f5fdf5; border: 1px solid #c8e6c8; border-radius: 6px; margin-bottom: 10px; padding: 15px; display: flex; justify-content: space-between; align-items: center; }
  .request-item span { font-size: 0.95em; color: #444; }
  .request-item strong { color: #000; }
  .request-actions { display: flex; gap: 10px; }
  .request-actions button { padding: 6px 12px; border: none; border-radius: 4px; cursor: pointer; font-size: 0.9em; }
  .request-actions .accept { background-color: #28a745; color: white; }
  .request-actions .accept:hover:not(:disabled) { background-color: #218838; }
  .request-actions .decline { background-color: #dc3545; color: white; }
  .request-actions .decline:hover:not(:disabled) { background-color: #c82333; }
  
  .crew-management-section { border-top: 2px solid #006400; }
  .crew-list-container .list-refresh-button { background-color: #ffc107; color: #333; }
  .crew-list-container .list-refresh-button:hover:not(:disabled) { background-color: #e0a800; }
  .sailor-item { align-items: flex-start; background-color: #ffffff; border-color: #e0e0e0; }
  .sailor-info { flex-grow: 1; margin-right: 15px; }
  .sailor-info > span { display: block; font-size: 1.05em; color: #333; margin-bottom: 8px; }
  .sailor-info > span strong { color: #006400; }
  .edit-description-active { margin-top: 10px; border-top: 1px dashed #ccc; padding-top: 10px; }
  .edit-description-active textarea { width: 100%; padding: 8px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; margin-bottom: 8px; min-height: 60px; }
  .edit-description-active .edit-actions { display: flex; justify-content: flex-end; gap: 8px; }
  .edit-description-active .edit-actions button { padding: 5px 10px; font-size: 0.9em; border-radius: 4px; border: none; cursor: pointer; }
  .edit-actions .save-button { background-color: #5cb85c; color: white; }
  .edit-actions .save-button:hover:not(:disabled) { background-color: #4cae4c; }
  .edit-actions .cancel-button { background-color: #f0ad4e; color: white; }
  .edit-actions .cancel-button:hover { background-color: #ec971f; }
  .sailor-actions { flex-shrink: 0; align-self: center; }
  .sailor-actions button { min-width: 130px; text-align: center; }
  .edit-desc-button { background-color: #f0ad4e; color: white; }
  .edit-desc-button:hover:not(:disabled) { background-color: #ec971f; }
  .inline-error { margin-top: 8px; font-size: 0.9em; padding: 5px; text-align: left; color: #721c24; background-color: #f8d7da; border: 1px solid #f5c6cb; border-radius: 3px;}
  .inline-success { margin-top: 8px; font-size: 0.9em; padding: 5px; text-align: left; color: #155724; background-color: #d4edda; border: 1px solid #c3e6cb; border-radius: 3px;}
  
  
  .loading { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #555; font-style: italic; }
  .error-message { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #721c24; background-color: #f8d7da; border: 1px solid #f5c6cb; }
  .success-message { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #155724; background-color: #d4edda; border: 1px solid #c3e6cb;}
  .no-data { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #666; font-style: italic; }
  
  </style>