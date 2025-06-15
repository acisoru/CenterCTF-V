<template>
    <div class="dashboard-container admiral-dashboard">
      <h1>Панель Управления Адмирала</h1>
      <p>Добро пожаловать, Адмирал {{ authStore.userName }}!</p>
  
      <div v-if="loadingFleetInfo" class="loading">Загрузка информации о флоте...</div>
      <div v-else-if="fleetError" class="error-message">{{ fleetError }}</div>
      <p v-else class="fleet-info-display">
          Ваш Флот: <strong>{{ fleetInfo.country || 'Не определен' }}</strong>
          <span v-if="fleetInfo.description">({{ fleetInfo.description }})</span>
      </p>
  
      <section class="requests-section">
        <h2>Прошения о Зачислении в Капитаны</h2>
        <button @click="fetchCaptainRequests" :disabled="loadingRequests" class="refresh-button section-refresh-button">
           {{ loadingRequests ? 'Обновление...' : 'Обновить Список Прошений' }}
        </button>
        <div v-if="loadingRequests" class="loading">Загрузка прошений...</div>
        <div v-else-if="requestsError" class="error-message">{{ requestsError }}</div>
        <ul v-else-if="captainRequests.length > 0" class="request-list">
          <li v-for="request in captainRequests" :key="request.id" class="request-item">
            <span>Прошение от Пользователя ID: <strong>{{ request.asker_id }}</strong></span>
            <div class="request-actions">
              <button @click="handleAcceptCaptain(request.asker_id)" class="accept" :disabled="processingRequest === request.asker_id">
                 {{ processingRequest === request.asker_id ? 'Принятие...' : 'Принять в Капитаны' }}
              </button>
              <button @click="handleDeclineCaptain(request.asker_id)" class="decline" :disabled="processingRequest === request.asker_id">
                 {{ processingRequest === request.asker_id ? 'Отклонение...' : 'Отклонить' }}
              </button>
            </div>
          </li>
        </ul>
        <div v-else class="no-data">Нет ожидающих прошений на должность капитана.</div>
         <p v-if="requestActionError" class="error-message">{{ requestActionError }}</p>
         <p v-if="requestActionSuccess" class="success-message">{{ requestActionSuccess }}</p>
      </section>
  
      <section class="ship-management-section">
          <h2>Управление Кораблями Флота</h2>
  
          <form @submit.prevent="handleCreateShip" class="create-ship-form sub-section-form">
              <h3>Добавить Новый Корабль</h3>
              <div class="form-group">
                  <label for="ship-name">Название Корабля:</label>
                  <input type="text" id="ship-name" v-model="newShipName" required placeholder="Например, 'Ингерманланд'">
              </div>
              <button type="submit" :disabled="creatingShip">
                   {{ creatingShip ? 'Создание...' : 'Добавить Корабль' }}
              </button>
              <p v-if="createShipError" class="error-message">{{ createShipError }}</p>
               <p v-if="createShipSuccess" class="success-message">{{ createShipSuccess }}</p>
          </form>
  
          <div class="list-container ship-list-container">
              <h3>Список Кораблей Флота</h3>
               <button @click="fetchShips" :disabled="loadingShips" class="refresh-button list-refresh-button">
                  {{ loadingShips ? 'Обновление...' : 'Обновить Список' }}
              </button>
              <div v-if="loadingShips" class="loading">Загрузка кораблей...</div>
              <div v-else-if="shipsError" class="error-message">{{ shipsError }}</div>
              <ul v-else-if="ships.length > 0" class="data-list ship-list">
                  <li v-for="ship in ships" :key="ship.id" class="data-item ship-item">
                      <span><strong>{{ ship.name }}</strong> (ID: {{ ship.id }})</span>
                      <button
                          @click="handleRemoveShip(ship.id)"
                          class="action-button remove-ship-button"
                          :disabled="removingShipId === ship.id">
                          {{ removingShipId === ship.id ? 'Удаление...' : 'Удалить' }}
                      </button>
                  </li>
              </ul>
              <div v-else class="no-data">В вашем флоте пока нет кораблей.</div>
              <p v-if="removeShipError" class="error-message">{{ removeShipError }}</p>
              <p v-if="removeShipSuccess" class="success-message">{{ removeShipSuccess }}</p>
          </div>
      </section>
  
      <section class="captain-management-section">
          <h2>Управление Капитанами Флота</h2>
  
          <form @submit.prevent="handleAssignCaptain" class="assign-captain-form sub-section-form">
              <h3>Назначить Капитана на Корабль</h3>
              <div class="form-group">
                  <label for="assign-captain-select">Капитан:</label>
                  <select id="assign-captain-select" v-model="selectedCaptainId" required :disabled="captains.length === 0">
                      <option disabled value="">-- Выберите капитана --</option>
                      <option v-for="captain in captains" :key="captain.id" :value="captain.id">
                          {{ captain.name }} (ID: {{ captain.id }})
                      </option>
                  </select>
              </div>
              <div class="form-group">
                  <label for="assign-ship-select">Корабль:</label>
                  <select id="assign-ship-select" v-model="selectedShipId" required :disabled="ships.length === 0">
                       <option disabled value="">-- Выберите корабль --</option>
                       <option v-for="ship in ships" :key="ship.id" :value="ship.id">
                          {{ ship.name }} (ID: {{ ship.id }})
                      </option>
                  </select>
              </div>
               <button type="submit" :disabled="assigningCaptain || !selectedCaptainId || !selectedShipId">
                  {{ assigningCaptain ? 'Назначение...' : 'Назначить' }}
              </button>
              <p v-if="assignCaptainError" class="error-message">{{ assignCaptainError }}</p>
              <p v-if="assignCaptainSuccess" class="success-message">{{ assignCaptainSuccess }}</p>
          </form>
  
          <div class="list-container captain-list-container">
              <h3>Список Капитанов Флота</h3>
               <button @click="fetchCaptains" :disabled="loadingCaptains" class="refresh-button list-refresh-button">
                  {{ loadingCaptains ? 'Обновление...' : 'Обновить Список' }}
              </button>
              <div v-if="loadingCaptains" class="loading">Загрузка капитанов...</div>
              <div v-else-if="captainsError" class="error-message">{{ captainsError }}</div>
              <ul v-else-if="captains.length > 0" class="data-list captain-list">
                  <li v-for="captain in captains" :key="captain.id" class="data-item captain-item">
                      <div class="captain-info">
                          <span><strong>{{ captain.name }}</strong> (ID: {{ captain.id }})</span>
                          <div v-if="editingCaptainId === captain.id" class="edit-description-active">
                               <textarea
                                  v-model="editingCaptainDescription"
                                  placeholder="Введите описание..."
                                  rows="2"
                               ></textarea>
                               <div class="edit-actions">
                                  <button @click="saveCaptainDescription(captain.id)" :disabled="savingCaptainDescription" class="action-button save-button">
                                      {{ savingCaptainDescription ? 'Сохр...' : 'Сохранить' }}
                                  </button>
                                  <button @click="cancelEditingDescription" class="action-button cancel-button">Отмена</button>
                               </div>
                          </div>
                      </div>
                      <div class="captain-actions">
                          <button
                              @click="startEditingDescription(captain.id)"
                              v-if="editingCaptainId !== captain.id"
                              class="action-button edit-desc-button"
                              title="Редактировать описание">
                              Описание
                          </button>
                          <button
                              @click="handleRemoveCaptainAssignment(captain.id)"
                              class="action-button remove-assignment-button"
                              :disabled="removingCaptainAssignmentId === captain.id"
                              title="Снять с корабля (если назначен)">
                              Снять назначение
                          </button>
                      </div>
                  </li>
              </ul>
               <div v-else class="no-data">В вашем флоте пока нет капитанов.</div>
               <p v-if="removeCaptainAssignmentError" class="error-message">{{ removeCaptainAssignmentError }}</p>
               <p v-if="removeCaptainAssignmentSuccess" class="success-message">{{ removeCaptainAssignmentSuccess }}</p>
               <p v-if="changeDescriptionError" class="error-message">{{ changeDescriptionError }}</p>
               <p v-if="changeDescriptionSuccess" class="success-message">{{ changeDescriptionSuccess }}</p>
          </div>
      </section>
  
       <section class="mission-management-section">
          <h2>Управление Миссиями Флота</h2>
  
          <form @submit.prevent="handleAssignMission" class="assign-mission-form sub-section-form">
              <h3>Назначить Новую Миссию</h3>
              <div class="form-group">
                  <label for="assign-mission-ship-select">Корабль:</label>
                  <select id="assign-mission-ship-select" v-model="selectedShipIdForMission" required :disabled="ships.length === 0">
                       <option disabled value="">-- Выберите корабль --</option>
                       <option v-for="ship in ships" :key="ship.id" :value="ship.id">
                          {{ ship.name }} (ID: {{ ship.id }})
                      </option>
                  </select>
              </div>
               <div class="form-group text-area-group">
                  <label for="mission-description">Описание Миссии:</label>
                  <textarea
                      id="mission-description"
                      v-model="newMissionDescription"
                      rows="3"
                      required
                      placeholder="Подробно опишите задачу для корабля..."
                  ></textarea>
              </div>
               <button type="submit" :disabled="assigningMission || !selectedShipIdForMission || !newMissionDescription">
                  {{ assigningMission ? 'Назначение...' : 'Назначить Миссию' }}
              </button>
              <p v-if="assignMissionError" class="error-message">{{ assignMissionError }}</p>
              <p v-if="assignMissionSuccess" class="success-message">{{ assignMissionSuccess }}</p>
          </form>
  
          <div class="list-container mission-list-container">
              <h3>Текущие Миссии</h3>
               <button @click="fetchMissions" :disabled="loadingMissions" class="refresh-button list-refresh-button">
                  {{ loadingMissions ? 'Обновление...' : 'Обновить Список' }}
              </button>
              <div v-if="loadingMissions" class="loading">Загрузка миссий...</div>
              <div v-else-if="missionsError" class="error-message">{{ missionsError }}</div>
              <ul v-else-if="missions.length > 0" class="data-list mission-list">
                  <li v-for="mission in missions" :key="mission.id" class="data-item mission-item" :class="{ 'completed': !mission.active }">
                      <div class="mission-details">
                          <p><strong>Миссия ID:</strong> {{ mission.id }}</p>
                          <p><strong>Корабль:</strong> {{ getShipNameById(mission.ship_id) || `ID ${mission.ship_id}` }}</p>
                          <p><strong>Статус:</strong> <span :class="mission.active ? 'status-completed': 'status-active'">{{ mission.active ? 'Завершена' : 'Активна' }}</span></p>
                          <p class="mission-description-text"><strong>Описание:</strong> {{ mission.description }}</p>
                      </div>
                      <button
                          @click="handleRemoveMission(mission.id)"
                          class="action-button remove-mission-button"
                          :disabled="removingMissionId === mission.id"
                          title="Удалить миссию">
                          {{ removingMissionId === mission.id ? 'Удаление...' : 'Удалить' }}
                      </button>
                  </li>
              </ul>
              <div v-else class="no-data">Активных или завершенных миссий не найдено.</div>
              <p v-if="removeMissionError" class="error-message">{{ removeMissionError }}</p>
              <p v-if="removeMissionSuccess" class="success-message">{{ removeMissionSuccess }}</p>
          </div>
       </section>
  
    </div>
  </template>
  
  <script setup>
  import { ref, onMounted, computed } from 'vue';
  import apiClient from '@/services/api';
  import { useAuthStore } from '@/stores/auth';
  import { decode, encode } from '@/utils/base64';
  
  const authStore = useAuthStore();
  
  const fleetInfo = ref({ id: null, country: null, description: null });
  const loadingFleetInfo = ref(false);
  const fleetError = ref(null);
  
  const captainRequests = ref([]);
  const loadingRequests = ref(false);
  const requestsError = ref(null);
  const processingRequest = ref(null);
  const requestActionError = ref(null);
  const requestActionSuccess = ref(null);
  
  const ships = ref([]);
  const loadingShips = ref(false);
  const shipsError = ref(null);
  const newShipName = ref('');
  const creatingShip = ref(false);
  const createShipError = ref(null);
  const createShipSuccess = ref(null);
  const removingShipId = ref(null);
  const removeShipError = ref(null);
  const removeShipSuccess = ref(null);
  
  const captains = ref([]);
  const loadingCaptains = ref(false);
  const captainsError = ref(null);
  const selectedCaptainId = ref('');
  const selectedShipId = ref('');
  const assigningCaptain = ref(false);
  const assignCaptainError = ref(null);
  const assignCaptainSuccess = ref(null);
  const removingCaptainAssignmentId = ref(null);
  const removeCaptainAssignmentError = ref(null);
  const removeCaptainAssignmentSuccess = ref(null);
  const editingCaptainId = ref(null);
  const editingCaptainDescription = ref('');
  const savingCaptainDescription = ref(false);
  const changeDescriptionError = ref(null);
  const changeDescriptionSuccess = ref(null);
  
  const missions = ref([]);
  const loadingMissions = ref(false);
  const missionsError = ref(null);
  const selectedShipIdForMission = ref('');
  const newMissionDescription = ref('');
  const assigningMission = ref(false);
  const assignMissionError = ref(null);
  const assignMissionSuccess = ref(null);
  const removingMissionId = ref(null);
  const removeMissionError = ref(null);
  const removeMissionSuccess = ref(null);
  
  
  const fetchAdmiralFleetInfo = async () => {
      loadingFleetInfo.value = true; fleetError.value = null;
      fleetInfo.value = { id: null, country: null, description: null };
      try {
          const response = await apiClient.get('/admiral/my_fleet');
          if (response.data && response.status === 200) {
              fleetInfo.value = { id: response.data.id, country: decode(response.data.country), description: decode(response.data.description) };
          } else { fleetError.value = decode(response.data?.message) || 'Флот не найден.'; }
      } catch (error) {
          console.error("Err fetch fleet:", error); fleetError.value = "Ошибка загрузки флота.";
          if (error.response?.data?.message) { try { fleetError.value = decode(error.response.data.message) || fleetError.value; } catch {} }
          else if (error.response?.status === 404) { fleetError.value = 'Флот не найден (404).'; }
      } finally { loadingFleetInfo.value = false; }
  };
  const fetchCaptainRequests = async () => {
    loadingRequests.value = true; requestsError.value = null; requestActionError.value = null; requestActionSuccess.value = null; captainRequests.value = [];
    try {
      const response = await apiClient.get('/admiral/get_all_captains_requests');
      if (response.data?.requests) { captainRequests.value = response.data.requests; }
      else { requestsError.value = decode(response.data?.message) || "Не удалось загрузить прошения."; }
    } catch (error) { console.error("Err fetch reqs:", error); requestsError.value = "Ошибка загрузки прошений."; if (error.response?.data?.message) { try { requestsError.value = decode(error.response.data.message) || requestsError.value; } catch {} } }
    finally { loadingRequests.value = false; }
  };
  const fetchShips = async () => {
      loadingShips.value = true; shipsError.value = null; ships.value = [];
      try {
          const response = await apiClient.get('/admiral/get_all_ships');
          if (response.data?.ships) { ships.value = response.data.ships.map(ship => ({ id: ship.id, name: decode(ship.name) })); }
          else if (response.data?.message) { console.log('Ships:', decode(response.data.message)); }
      } catch (error) { console.error("Err fetch ships:", error); shipsError.value = "Ошибка загрузки кораблей."; if (error.response?.data?.message) { try { shipsError.value = decode(error.response.data.message) || shipsError.value; } catch {} } }
      finally { loadingShips.value = false; }
  };
  const fetchCaptains = async () => {
      loadingCaptains.value = true; captainsError.value = null; captains.value = [];
      try {
          const response = await apiClient.get('/admiral/get_all_captains');
          if (response.data?.captains) { captains.value = response.data.captains.map(c => ({ id: c.id, name: decode(c.name) })); }
          else if (response.data?.message) { console.log('Captains:', decode(response.data.message)); }
      } catch (error) { console.error("Err fetch captains:", error); captainsError.value = "Ошибка загрузки капитанов."; if (error.response?.data?.message) { try { captainsError.value = decode(error.response.data.message) || captainsError.value; } catch {} } }
      finally { loadingCaptains.value = false; }
  };
  const fetchMissions = async () => {
      loadingMissions.value = true; missionsError.value = null; missions.value = [];
      try {
          const response = await apiClient.get('/admiral/get_all_missions');
          if (response.data?.missions) {
              missions.value = response.data.missions.map(m => ({ id: m.id, description: decode(m.description), active: m.active, ship_id: m.ship_id }))
                             .sort((a, b) => a.active === b.active ? a.id - b.id : (a.active ? -1 : 1));
          } else if (response.data?.message) { console.log('Missions:', decode(response.data.message)); }
      } catch (error) { console.error("Err fetch missions:", error); missionsError.value = "Ошибка загрузки миссий."; if (error.response?.data?.message) { try { missionsError.value = decode(error.response.data.message) || missionsError.value; } catch {} } }
      finally { loadingMissions.value = false; }
  };
  
  const handleAcceptCaptain = async (captainId) => {
      processingRequest.value = captainId; requestActionError.value = null; requestActionSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/accept_captain_request', { captain_id: captainId });
          requestActionSuccess.value = decode(response.data?.message) || 'Прошение одобрено.';
          await fetchCaptainRequests(); await fetchCaptains();
      } catch (error) { console.error("Err accept cap:", error); requestActionError.value = "Ошибка одобрения."; if (error.response?.data?.message) { try { requestActionError.value = decode(error.response.data.message) || requestActionError.value; } catch {} } }
      finally { processingRequest.value = null; setTimeout(() => { requestActionSuccess.value = null; requestActionError.value = null;}, 4000); }
  };
  const handleDeclineCaptain = async (captainId) => {
       processingRequest.value = captainId; requestActionError.value = null; requestActionSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/decline_captain_request', { captain_id: captainId });
           requestActionSuccess.value = decode(response.data?.message) || 'Прошение отклонено.';
          await fetchCaptainRequests();
      } catch (error) { console.error("Err decline cap:", error); requestActionError.value = "Ошибка отклонения."; if (error.response?.data?.message) { try { requestActionError.value = decode(error.response.data.message) || requestActionError.value; } catch {} } }
      finally { processingRequest.value = null; setTimeout(() => { requestActionSuccess.value = null; requestActionError.value = null; }, 4000); }
  };
  const handleCreateShip = async () => {
      if (!newShipName.value.trim()) { createShipError.value = "Название не может быть пустым."; return; }
      creatingShip.value = true; createShipError.value = null; createShipSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/create_ship', { ship_name: encode(newShipName.value.trim()) });
          createShipSuccess.value = decode(response.data?.message) || "Корабль создан.";
          newShipName.value = ''; await fetchShips();
      } catch (error) { console.error("Err create ship:", error); createShipError.value = "Ошибка создания корабля."; if (error.response?.data?.message) { try { createShipError.value = decode(error.response.data.message) || createShipError.value; } catch {} } }
      finally { creatingShip.value = false; setTimeout(() => { createShipSuccess.value = null; createShipError.value = null; }, 4000); }
  };
  const handleRemoveShip = async (shipId) => {
      if (!confirm(`Удалить корабль ID ${shipId}?`)) { return; }
      removingShipId.value = shipId; removeShipError.value = null; removeShipSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/remove_ship', { ship_id: shipId });
          removeShipSuccess.value = decode(response.data?.message) || "Корабль удален.";
          await fetchShips(); await fetchMissions();
      } catch (error) { console.error("Err remove ship:", error); removeShipError.value = `Ошибка удаления ID ${shipId}.`; if (error.response?.data?.message) { try { removeShipError.value = decode(error.response.data.message) || removeShipError.value; } catch {} } }
      finally { removingShipId.value = null; setTimeout(() => { removeShipSuccess.value = null; removeShipError.value = null; }, 4000); }
  };
  const handleAssignCaptain = async () => {
      if (!selectedCaptainId.value || !selectedShipId.value) { assignCaptainError.value = "Выберите капитана и корабль."; return; }
      assigningCaptain.value = true; assignCaptainError.value = null; assignCaptainSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/assign_captain_to_ship', { captain_id: selectedCaptainId.value, ship_id: selectedShipId.value });
          assignCaptainSuccess.value = decode(response.data?.message) || "Капитан назначен.";
          selectedCaptainId.value = ''; selectedShipId.value = '';
      } catch (error) { console.error("Err assign cap:", error); assignCaptainError.value = "Ошибка назначения."; if (error.response?.data?.message) { try { assignCaptainError.value = decode(error.response.data.message) || assignCaptainError.value; } catch {} } }
      finally { assigningCaptain.value = false; setTimeout(() => { assignCaptainSuccess.value = null; assignCaptainError.value = null; }, 4000); }
  };
  const handleRemoveCaptainAssignment = async (captainId) => {
       if (!confirm(`Снять назначение с капитана ID ${captainId}?`)) { return; }
      removingCaptainAssignmentId.value = captainId; removeCaptainAssignmentError.value = null; removeCaptainAssignmentSuccess.value = null;
      try {
           const response = await apiClient.post('/admiral/remove_captain_from_ship', { captain_id: captainId });
           removeCaptainAssignmentSuccess.value = decode(response.data?.message) || "Назначение снято.";
      } catch (error) { console.error("Err remove assign:", error); removeCaptainAssignmentError.value = `Ошибка снятия ID ${captainId}.`; if (error.response?.data?.message) { try { removeCaptainAssignmentError.value = decode(error.response.data.message) || removeCaptainAssignmentError.value; } catch {} } }
      finally { removingCaptainAssignmentId.value = null; setTimeout(() => { removeCaptainAssignmentSuccess.value = null; removeCaptainAssignmentError.value = null; }, 4000); }
  };
  const startEditingDescription = (captainId) => {
      editingCaptainDescription.value = '';
      editingCaptainId.value = captainId;
      changeDescriptionError.value = null; changeDescriptionSuccess.value = null;
  };
  const cancelEditingDescription = () => {
      editingCaptainId.value = null; editingCaptainDescription.value = '';
  };
  const saveCaptainDescription = async (captainId) => {
      savingCaptainDescription.value = true; changeDescriptionError.value = null; changeDescriptionSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/change_captain_description', { captain_id: captainId, new_description: encode(editingCaptainDescription.value) });
          changeDescriptionSuccess.value = decode(response.data?.message) || "Описание изменено.";
          editingCaptainId.value = null;
      } catch (error) { console.error("Err change desc:", error); changeDescriptionError.value = `Ошибка описания ID ${captainId}.`; if (error.response?.data?.message) { try { changeDescriptionError.value = decode(error.response.data.message) || changeDescriptionError.value; } catch {} } }
      finally { savingCaptainDescription.value = false; setTimeout(() => { changeDescriptionSuccess.value = null; changeDescriptionError.value = null; }, 4000); }
  };
  const handleAssignMission = async () => {
      if (!selectedShipIdForMission.value || !newMissionDescription.value.trim()) { assignMissionError.value = "Выберите корабль и введите описание."; return; }
      assigningMission.value = true; assignMissionError.value = null; assignMissionSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/assign_mission_to_ship', { ship_id: selectedShipIdForMission.value, description: encode(newMissionDescription.value.trim()) });
          assignMissionSuccess.value = decode(response.data?.message) || "Миссия назначена.";
          selectedShipIdForMission.value = ''; newMissionDescription.value = '';
          await fetchMissions();
      } catch (error) { console.error("Err assign mission:", error); assignMissionError.value = "Ошибка назначения миссии."; if (error.response?.data?.message) { try { assignMissionError.value = decode(error.response.data.message) || assignMissionError.value; } catch {} } }
      finally { assigningMission.value = false; setTimeout(() => { assignMissionSuccess.value = null; assignMissionError.value = null; }, 4000); }
  };
  const handleRemoveMission = async (missionId) => {
      if (!confirm(`Удалить миссию ID ${missionId}?`)) { return; }
      removingMissionId.value = missionId; removeMissionError.value = null; removeMissionSuccess.value = null;
      try {
          const response = await apiClient.post('/admiral/remove_mission_to_ship', { mission_id: missionId });
          removeMissionSuccess.value = decode(response.data?.message) || "Миссия удалена.";
          await fetchMissions();
      } catch (error) { console.error("Err remove mission:", error); removeMissionError.value = `Ошибка удаления ID ${missionId}.`; if (error.response?.data?.message) { try { removeMissionError.value = decode(error.response.data.message) || removeMissionError.value; } catch {} } }
      finally { removingMissionId.value = null; setTimeout(() => { removeMissionSuccess.value = null; removeMissionError.value = null; }, 4000); }
  };
  
  const getShipNameById = (shipId) => {
      const ship = ships.value.find(s => s.id === shipId);
      return ship ? ship.name : null;
  };
  
  onMounted(() => {
    fetchAdmiralFleetInfo();
    fetchCaptainRequests();
    fetchShips();
    fetchCaptains();
    fetchMissions();
  });
  </script>
  
  <style scoped>
  .dashboard-container { padding: 20px; font-family: sans-serif; max-width: 1100px; margin: 20px auto; background-color: #f8f0f0; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
  h1, h2 { color: #8b0000; text-align: center; margin-bottom: 15px; }
  h2 { margin-top: 30px; border-bottom: 1px solid #ccc; padding-bottom: 10px; position: relative;}
  h3 { color: #a0522d; margin-top: 20px; margin-bottom: 15px; text-align: center; font-size: 1.2em; }
  p { text-align: center; color: #333; margin-bottom: 20px; }
  section { margin-bottom: 40px; padding: 25px; background-color: #fff; border-radius: 6px; border: 1px solid #ddd; box-shadow: 0 2px 5px rgba(0,0,0,0.05);}
  button:disabled { opacity: 0.6; cursor: not-allowed; }
  
  .refresh-button { padding: 6px 12px; font-size: 0.9em; background-color: #6c757d; color: white; border: none; border-radius: 4px; cursor: pointer; transition: background-color 0.2s; }
  .refresh-button:hover:not(:disabled) { background-color: #5a6268; }
  .section-refresh-button { display: block; margin: 0 auto 20px auto; padding: 8px 15px; background-color: #007bff; }
  .section-refresh-button:hover:not(:disabled) { background-color: #0056b3; }
  .list-refresh-button { position: absolute; top: -35px; right: 0; }
  
  .action-button { padding: 6px 12px; border: none; border-radius: 4px; cursor: pointer; font-size: 0.9em; transition: background-color 0.2s; }
  
  .sub-section-form { background-color: #fdfdfd; padding: 20px; border-radius: 6px; border: 1px solid #eee; margin-bottom: 30px; box-shadow: inset 0 1px 3px rgba(0,0,0,0.05); }
  .sub-section-form .form-group { margin-bottom: 15px; display: flex; align-items: center; gap: 10px; }
  .sub-section-form label { flex-basis: 150px; text-align: right; font-weight: bold; color: #555; flex-shrink: 0; }
  .sub-section-form input[type="text"], .sub-section-form select, .sub-section-form textarea { flex-grow: 1; padding: 10px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
  .sub-section-form textarea { resize: vertical; min-height: 60px; }
  .sub-section-form .text-area-group { align-items: flex-start; }
  .sub-section-form .text-area-group label { padding-top: 5px; }
  .sub-section-form button[type="submit"] { display: block; margin: 15px auto 0 auto; padding: 10px 25px; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 1em; }
  
  .list-container { position: relative; margin-top: 20px; padding-top: 10px; border-top: 1px solid #eee; }
  .data-list { list-style: none; padding: 0; margin-top: 10px; }
  .data-item { display: flex; justify-content: space-between; align-items: center; padding: 12px 15px; border: 1px solid #eee; border-radius: 4px; margin-bottom: 8px; background-color: #f9f9f9; }
  .data-item:nth-child(odd) { background-color: #fefefe; }
  
  .fleet-info-display { font-size: 1.1em; margin-top: 5px; margin-bottom: 25px; color: #555; }
  .fleet-info-display strong { color: #000; }
  .fleet-info-display span { font-style: italic; color: #666; }
  
  .request-list { list-style: none; padding: 0; }
  .request-item { background-color: #fdf5f5; border: 1px solid #e6c8c8; border-radius: 6px; margin-bottom: 10px; padding: 15px; display: flex; justify-content: space-between; align-items: center; }
  .request-item span { font-size: 0.95em; color: #444; }
  .request-item strong { color: #000; }
  .request-actions { display: flex; gap: 10px; }
  .request-actions button { padding: 6px 12px; border: none; border-radius: 4px; cursor: pointer; font-size: 0.9em; }
  .request-actions .accept { background-color: #28a745; color: white; }
  .request-actions .accept:hover:not(:disabled) { background-color: #218838; }
  .request-actions .decline { background-color: #dc3545; color: white; }
  .request-actions .decline:hover:not(:disabled) { background-color: #c82333; }
  
  .create-ship-form { background-color: #fff8f0; border-color: #e0d8cc; }
  .create-ship-form button[type="submit"] { background-color: #5cb85c; }
  .create-ship-form button[type="submit"]:hover:not(:disabled) { background-color: #4cae4c; }
  .ship-item span strong { color: #1a237e; }
  .remove-ship-button { background-color: #d9534f; color: white; margin-left: 15px; }
  .remove-ship-button:hover:not(:disabled) { background-color: #c9302c; }
  
  .assign-captain-form { background-color: #f0f8ff; border-color: #cce0ff; }
  .assign-captain-form button[type="submit"] { background-color: #007bff; }
  .assign-captain-form button[type="submit"]:hover:not(:disabled) { background-color: #0056b3; }
  .captain-item { align-items: flex-start; }
  .captain-info { flex-grow: 1; margin-right: 15px; }
  .captain-info > span { display: block; font-size: 1.05em; color: #333; margin-bottom: 8px; }
  .captain-info > span strong { color: #8b0000; }
  .edit-description-active { margin-top: 10px; border-top: 1px dashed #ccc; padding-top: 10px; }
  .edit-description-active textarea { width: 100%; padding: 8px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; margin-bottom: 8px; min-height: 50px; }
  .edit-description-active .edit-actions { display: flex; justify-content: flex-end; gap: 8px; }
  .edit-description-active .edit-actions button { padding: 5px 10px; font-size: 0.9em; border-radius: 4px; border: none; cursor: pointer; }
  .edit-actions .save-button { background-color: #5cb85c; color: white; }
  .edit-actions .save-button:hover:not(:disabled) { background-color: #4cae4c; }
  .edit-actions .cancel-button { background-color: #f0ad4e; color: white; }
  .edit-actions .cancel-button:hover { background-color: #ec971f; }
  .captain-actions { display: flex; flex-direction: column; gap: 8px; flex-shrink: 0; }
  .captain-actions button { min-width: 130px; text-align: center; }
  .edit-desc-button { background-color: #f0ad4e; color: white; }
  .edit-desc-button:hover:not(:disabled) { background-color: #ec971f; }
  .remove-assignment-button { background-color: #d9534f; color: white; }
  .remove-assignment-button:hover:not(:disabled) { background-color: #c9302c; }
  
  .assign-mission-form { background-color: #f5f5f5; border-color: #e0e0e0; }
  .assign-mission-form .form-group label { flex-basis: 140px; }
  .assign-mission-form button[type="submit"] { background-color: #ff8c00; }
  .assign-mission-form button[type="submit"]:hover:not(:disabled) { background-color: #cc7000; }
  .mission-item { border-left: 5px solid #ff8c00; background-color: #fffafa; }
  .mission-item.completed { border-left-color: #90ee90; background-color: #f8fff8; }
  .mission-details { flex-grow: 1; margin-right: 15px; }
  .mission-details p { text-align: left; margin-bottom: 8px; line-height: 1.4; color: #444; }
  .mission-details p:last-child { margin-bottom: 0; }
  .mission-details strong { color: #333; min-width: 80px; display: inline-block; }
  .mission-description-text { white-space: pre-wrap; word-break: break-word; }
  .status-active { color: #ff8c00; font-weight: bold; }
  .status-completed { color: #2e8b57; font-weight: bold; }
  .remove-mission-button { background-color: #dc3545; color: white; flex-shrink: 0; align-self: center; margin-left: 15px; }
  .remove-mission-button:hover:not(:disabled) { background-color: #c82333; }
  
  
  .loading { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #555; font-style: italic; }
  .error-message { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #721c24; background-color: #f8d7da; border: 1px solid #f5c6cb; }
  .success-message { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #155724; background-color: #d4edda; border: 1px solid #c3e6cb;}
  .no-data { text-align: center; margin-top: 15px; padding: 10px; border-radius: 4px; color: #666; font-style: italic; }
  
  </style>