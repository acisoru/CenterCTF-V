import axios from 'axios';
import { useAuthStore } from '@/stores/auth';
import { decode } from '@/utils/base64';

const apiClient = axios.create({
  baseURL: '/api',
  headers: {
    'Content-Type': 'application/json',
  },
  withCredentials: true
});

apiClient.interceptors.request.use(
  (config) => {
    const authStore = useAuthStore();
    const token = authStore.token;

    if (token) {
       config.headers['Authorization'] = `Bearer ${token}`;
    }
    return config;
  },
  (error) => {
    return Promise.reject(error);
  }
);


apiClient.interceptors.response.use(
  (response) => {
    return response;
  },
  (error) => {
    if (error.response) {
      if (error.response.status === 401) {
        const authStore = useAuthStore(); 
        console.error('Unauthorized or Token Expired (via proxy):', error.response.data);

        let errorMessage = 'Ошибка аутентификации';
        if (error.response.data) {
            let potentialMessage = '';
            if (typeof error.response.data === 'string') {
                potentialMessage = error.response.data;
            } else if (error.response.data.message) {
                potentialMessage = error.response.data.message;
            }

            if (potentialMessage) {
                try {
                    const decoded = decode(potentialMessage);
                    errorMessage = decoded || potentialMessage;
                } catch (e) {
                    console.warn("Failed to decode error message from backend, using original.", e);
                    errorMessage = potentialMessage;
                }
            }
        }

        alert(`Ошибка авторизации: ${errorMessage}. Пожалуйста, войдите снова.`);
        authStore.logout();
      } else if (error.response.data) {
        let errorMessage = 'Произошла ошибка';
         let potentialMessage = '';
         if (typeof error.response.data === 'string') {
             potentialMessage = error.response.data;
         } else if (error.response.data.message) {
             potentialMessage = error.response.data.message;
         }
         if (potentialMessage) {
             try {
                 const decoded = decode(potentialMessage);
                 errorMessage = decoded || potentialMessage;
             } catch (e) {
                 errorMessage = potentialMessage;
             }
         }
        console.error('API Error (via proxy):', errorMessage, error.response);
      }
    } else if (error.request) {
      console.error('Network Error or No Response (via proxy):', error.request);
      alert('Сетевая ошибка или сервер недоступен. Проверьте подключение к интернету.');
    } else {
      console.error('Axios Setup Error (via proxy):', error.message);
    }

    return Promise.reject(error);
  }
);

export default apiClient;