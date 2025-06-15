<template>
    <div class="dashboard-container">
      <h1>Обзор Флотов Империи</h1>
      <p>Добро пожаловать, {{ authStore.userName }}!</p>
  
      <div class="actions">
         <button @click="showCreateFleetModal = true" class="action-button create-fleet">Основать Новый Флот</button>
         <p>Или</p>
         <button @click="showJoinRequestModal = true" class="action-button join-request">Подать Прошение о Службе</button>
      </div>
  
      <div class="fleet-list">
        <h2>Существующие Флоты</h2>
         <div v-if="loadingFleets" class="loading">Загрузка флотов...</div>
         <div v-else-if="fleetError" class="error-message">{{ fleetError }}</div>
        <ul v-else-if="fleets.length > 0">
          <li v-for="fleet in fleets" :key="fleet.id" class="fleet-item">
            <div class="fleet-info">
              <strong>{{ fleet.country }}</strong> <br>
              <span>{{ fleet.description }}</span>
            </div>
            <button @click="fetchCommand(fleet.id)" :disabled="loadingCommand === fleet.id">
               {{ loadingCommand === fleet.id ? 'Загрузка...' : 'Показать Командование' }}
            </button>
            <div v-if="selectedFleetId === fleet.id && command" class="command-info">
               <h4>Командование Флота "{{ fleet.country }}"</h4>
               <div v-if="commandLoading" class="loading">Загрузка...</div>
               <div v-else-if="commandError" class="error-message">{{ commandError }}</div>
               <ul v-else-if="command.length > 0">
                  <li v-for="person in command" :key="person.id">
                      {{ person.name }} (ID: {{ person.id }}) <!-- Show ID for request -->
                  </li>
               </ul>
               <div v-else>Нет данных о командовании.</div>
            </div>
          </li>
        </ul>
         <div v-else>Флоты еще не созданы.</div>
      </div>
  
      <!-- Create Fleet Modal -->
      <div v-if="showCreateFleetModal" class="modal-overlay" @click.self="showCreateFleetModal = false">
          <div class="modal-content">
              <h3>Основание Нового Флота</h3>
              <form @submit.prevent="handleCreateFleet">
                   <div class="form-group">
                      <label for="fleet-country">Название (Страна/Регион):</label>
                      <input type="text" id="fleet-country" v-model="newFleet.country" required>
                   </div>
                   <div class="form-group">
                      <label for="fleet-desc">Описание:</label>
                      <textarea id="fleet-desc" v-model="newFleet.description" rows="3" required></textarea>
                   </div>
                   <div class="modal-actions">
                      <button type="submit" :disabled="creatingFleet">
                          {{ creatingFleet ? 'Создание...' : 'Основать Флот' }}
                      </button>
                      <button type="button" @click="showCreateFleetModal = false" class="cancel">Отмена</button>
                   </div>
                   <p v-if="createFleetError" class="error-message">{{ createFleetError }}</p>
              </form>
          </div>
      </div>
  
       <!-- Join Request Modal -->
      <div v-if="showJoinRequestModal" class="modal-overlay" @click.self="showJoinRequestModal = false">
          <div class="modal-content">
              <h3>Прошение о Зачислении на Службу</h3>
              <form @submit.prevent="handleCreateRequest">
                   <div class="form-group">
                      <label for="accepter-id">ID Командира (Адмирала/Капитана):</label>
                      <input type="number" id="accepter-id" v-model.number="newRequest.accepterId" required placeholder="ID из списка командования">
                   </div>
                   <div class="form-group">
                      <label for="request-role">Желаемая Должность:</label>
                      <select id="request-role" v-model="newRequest.role" required>
                          <option value="Captain">Капитан</option>
                          <option value="Sailor">Матрос</option>
                      </select>
                   </div>
                    <p class="info-note">Примечание: Запрос на должность Капитана отправляется Адмиралу флота. Запрос на должность Матроса отправляется Капитану корабля.</p>
                   <div class="modal-actions">
                      <button type="submit" :disabled="creatingRequest">
                          {{ creatingRequest ? 'Отправка...' : 'Подать Прошение' }}
                      </button>
                      <button type="button" @click="showJoinRequestModal = false" class="cancel">Отмена</button>
                   </div>
                   <p v-if="createRequestError" class="error-message">{{ createRequestError }}</p>
                   <p v-if="createRequestSuccess" class="success-message">{{ createRequestSuccess }}</p>
              </form>
          </div>
      </div>
  
    </div>
  </template>
  
  <script setup>
  import { ref, onMounted } from 'vue';
  import apiClient from '@/services/api';
  import { useAuthStore } from '@/stores/auth';
  import { decode, encode } from '@/utils/base64';
  
  const authStore = useAuthStore();
  const fleets = ref([]);
  const loadingFleets = ref(false);
  const fleetError = ref(null);
  
  const command = ref(null);
  const selectedFleetId = ref(null);
  const loadingCommand = ref(null); // Store ID of fleet whose command is loading
  const commandLoading = ref(false); // General flag for command loading state
  const commandError = ref(null);
  
  const showCreateFleetModal = ref(false);
  const creatingFleet = ref(false);
  const createFleetError = ref(null);
  const newFleet = ref({ country: '', description: '' });
  
  const showJoinRequestModal = ref(false);
  const creatingRequest = ref(false);
  const createRequestError = ref(null);
  const createRequestSuccess = ref(null);
  const newRequest = ref({ accepterId: null, role: 'Sailor' }); // Default role
  
  
  const fetchFleets = async () => {
    loadingFleets.value = true;
    fleetError.value = null;
    fleets.value = []; // Clear previous fleets
    try {
      const response = await apiClient.get('/user/get_all_fleets');
      if (response.data && response.data.fleets) {
          // Decode fleet data
          fleets.value = response.data.fleets.map(f => ({
              ...f,
              country: decode(f.country),
              description: decode(f.description)
          }));
      } else {
          fleets.value = [];
      }
    } catch (error) {
      console.error("Error fetching fleets:", error);
      fleetError.value = "Не удалось загрузить список флотов.";
       if (error.response?.data?.message) {
           try { fleetError.value = decode(error.response.data.message) || fleetError.value; } catch { /* ignore */ }
       }
    } finally {
      loadingFleets.value = false;
    }
  };
  
  const fetchCommand = async (fleetId) => {
    if (selectedFleetId.value === fleetId && command.value) {
        // Toggle off if clicking the same button again
        selectedFleetId.value = null;
        command.value = null;
        return;
    }
  
    selectedFleetId.value = fleetId;
    loadingCommand.value = fleetId; // Indicate which command is loading
    commandLoading.value = true;
    commandError.value = null;
    command.value = null; // Clear previous command
  
    try {
      const response = await apiClient.get(`/user/get_command_for_fleet?fleet_id=${fleetId}`);
       if (response.data && response.data.command) {
          // Decode command names
          command.value = response.data.command.map(p => ({
              ...p,
              name: decode(p.name)
          }));
      } else {
          command.value = [];
          commandError.value = decode(response.data?.message) || "Нет данных о командовании.";
      }
    } catch (error) {
      console.error(`Error fetching command for fleet ${fleetId}:`, error);
       commandError.value = "Не удалось загрузить командование.";
        if (error.response?.data?.message) {
           try { commandError.value = decode(error.response.data.message) || commandError.value; } catch { /* ignore */ }
       }
    } finally {
      loadingCommand.value = null; // Reset loading indicator for this specific fleet
      commandLoading.value = false; // Reset general loading flag
    }
  };
  
  const handleCreateFleet = async () => {
      creatingFleet.value = true;
      createFleetError.value = null;
      try {
          const payload = {
              country: encode(newFleet.value.country),
              description: encode(newFleet.value.description)
          };
          const response = await apiClient.post('/admiral/create_fleet', payload);
          alert(decode(response.data?.message) || 'Флот успешно создан! Теперь вы Адмирал.');
          showCreateFleetModal.value = false;
          newFleet.value = { country: '', description: '' }; // Reset form
          await fetchFleets(); // Refresh fleet list
          // IMPORTANT: Need to update user role in authStore and redirect
          authStore.user.role = 'Admiral'; // Update local state
          localStorage.setItem('authUser', JSON.stringify(authStore.user)); // Update storage
          authStore.navigateUser(); // Navigate to admiral dashboard
  
      } catch (error) {
          console.error("Error creating fleet:", error);
          createFleetError.value = "Ошибка при создании флота.";
           if (error.response?.data?.message) {
               try { createFleetError.value = decode(error.response.data.message) || createFleetError.value; } catch { /* ignore */ }
           }
      } finally {
          creatingFleet.value = false;
      }
  };
  
  const handleCreateRequest = async () => {
      creatingRequest.value = true;
      createRequestError.value = null;
      createRequestSuccess.value = null;
  
      if (!newRequest.value.accepterId || !newRequest.value.role) {
          createRequestError.value = "Необходимо указать ID командира и желаемую должность.";
          creatingRequest.value = false;
          return;
      }
  
      try {
           const payload = {
              accepterId: newRequest.value.accepterId,
              role: encode(newRequest.value.role)
          };
          const response = await apiClient.post('/user/create_request', payload);
          createRequestSuccess.value = decode(response.data?.message) || 'Прошение успешно отправлено.';
          newRequest.value = { accepterId: null, role: 'Sailor' }; // Reset form
          // Optionally close modal after a delay
           setTimeout(() => {
              showJoinRequestModal.value = false;
               createRequestSuccess.value = null; // Clear success message
           }, 3000);
  
      } catch (error) {
           console.error("Error creating request:", error);
          createRequestError.value = "Ошибка при отправке прошения.";
           if (error.response?.data?.message) {
               try { createRequestError.value = decode(error.response.data.message) || createRequestError.value; } catch { /* ignore */ }
           }
      } finally {
          creatingRequest.value = false;
      }
  };
  
  
  onMounted(() => {
    fetchFleets();
  });
  </script>
  
  <style scoped>
  .dashboard-container {
  padding: 20px;
  font-family: sans-serif;
   /* max-width: 900px; */ /* REMOVE or increase */
   max-width: 1100px; /* INCREASED */
   margin: 20px auto;
   background-color: #f0f4f8; /* Light blue-grey background */
   border-radius: 8px;
   box-shadow: 0 4px 8px rgba(0,0,0,0.1);
}
  
  h1, h2, h4 {
    color: #1a237e; /* Dark Navy Blue */
     text-align: center;
     margin-bottom: 15px;
  }
  h2 {
     margin-top: 30px;
  }
  
  p {
     text-align: center;
     color: #333;
     margin-bottom: 20px;
  }
  
  .actions {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 15px;
      margin-bottom: 30px;
      padding: 15px;
      background-color: #e3eaf3;
      border-radius: 6px;
  }
  
  .action-button {
      padding: 10px 20px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 15px;
      font-weight: bold;
      transition: background-color 0.3s, transform 0.2s;
  }
  
  .action-button:hover {
      transform: translateY(-2px);
  }
  
  .create-fleet {
      background-color: #4CAF50; /* Green */
      color: white;
  }
  .create-fleet:hover {
      background-color: #45a049;
  }
  
  .join-request {
      background-color: #007bff; /* Blue */
      color: white;
  }
  .join-request:hover {
      background-color: #0056b3;
  }
  
  
  .fleet-list ul {
    list-style: none;
    padding: 0;
  }
  
  .fleet-item {
    background-color: #fff;
    border: 1px solid #ccc;
    border-radius: 6px;
    margin-bottom: 15px;
    padding: 15px 20px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    transition: box-shadow 0.3s ease;
  }
  .fleet-item:hover {
      box-shadow: 0 2px 5px rgba(0,0,0,0.15);
  }
  
  .fleet-info {
    flex-grow: 1;
    margin-right: 15px;
  }
  .fleet-info strong {
     font-size: 1.1em;
     color: #333;
  }
  .fleet-info span {
     color: #666;
     font-size: 0.9em;
  }
  
  
  .fleet-item button {
    padding: 8px 15px;
    background-color: #6c757d; /* Grey */
    color: white;
    border: none;
    border-radius: 4px;
    cursor: pointer;
    transition: background-color 0.2s;
  }
  
  .fleet-item button:hover:not(:disabled) {
    background-color: #5a6268;
  }
  .fleet-item button:disabled {
      background-color: #ccc;
      cursor: not-allowed;
  }
  
  .command-info {
    margin-top: 15px;
    padding: 15px;
    border: 1px dashed #aaa;
    border-radius: 5px;
    background-color: #f8f9fa;
    width: 100%; /* Make it span full width below button */
    box-sizing: border-box; /* Include padding in width */
    margin-left: -20px; /* Adjust to align with parent padding */
    margin-right: -20px;
    padding-left: 20px;
    padding-right: 20px;
  }
  
  .command-info ul {
      padding-left: 20px; /* Indent command list */
       list-style-type: disc;
  }
  .command-info li {
      margin-bottom: 5px;
      color: #444;
  }
  
  .loading, .error-message, .success-message {
    text-align: center;
    margin-top: 15px;
    padding: 10px;
    border-radius: 4px;
  }
  
  .loading {
      color: #555;
  }
  
  .error-message {
    color: #d9534f;
    background-color: #f2dede;
    border: 1px solid #ebccd1;
  }
  .success-message {
      color: #3c763d;
      background-color: #dff0d8;
      border: 1px solid #d6e9c6;
  }
  
  
  /* Modal Styles */
  .modal-overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background-color: rgba(0, 0, 0, 0.6);
    display: flex;
    justify-content: center;
    align-items: center;
    z-index: 1000;
  }
  
  .modal-content {
  background-color: white;
  padding: 30px;
  border-radius: 8px;
  width: 90%;
  /* max-width: 500px; */ /* REMOVE or increase */
  max-width: 650px; /* INCREASED */
  box-shadow: 0 5px 15px rgba(0,0,0,0.2);
}
  
  .modal-content h3 {
      text-align: center;
      margin-bottom: 25px;
      color: #1a237e;
  }
  
  .modal-content .form-group {
      margin-bottom: 15px;
  }
  .modal-content label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
      color: #555;
  }
  .modal-content input[type="text"],
  .modal-content input[type="number"],
  .modal-content textarea,
  .modal-content select {
      width: 100%;
      padding: 10px;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
  }
  .modal-content textarea {
      resize: vertical;
  }
  
  .modal-actions {
      display: flex;
      justify-content: flex-end;
      gap: 10px;
      margin-top: 25px;
  }
  
  .modal-actions button {
      padding: 10px 20px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 15px;
      border: none;
  }
  
  .modal-actions button[type="submit"] {
      background-color: #28a745;
      color: white;
  }
  .modal-actions button[type="submit"]:hover:not(:disabled) {
      background-color: #218838;
  }
  .modal-actions button[type="submit"]:disabled {
      background-color: #aaa;
      cursor: not-allowed;
  }
  
  
  .modal-actions button.cancel {
      background-color: #6c757d;
      color: white;
  }
  .modal-actions button.cancel:hover {
      background-color: #5a6268;
  }
  
  .info-note {
      font-size: 0.85em;
      color: #666;
      margin-top: 10px;
      margin-bottom: 15px;
      text-align: left;
      font-style: italic;
  }
  
  </style>