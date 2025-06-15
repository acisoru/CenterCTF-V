import { defineStore } from 'pinia';
import apiClient from '@/services/api';
import router from '@/router';
import { encode, decode } from '@/utils/base64';
import { jwtDecode } from 'jwt-decode';

export const useAuthStore = defineStore('auth', {
  state: () => ({
    token: localStorage.getItem('authToken') || null,
    user: JSON.parse(localStorage.getItem('authUser') || '{}'),
    status: null,
    error: null,
  }),
  getters: {
    isAuthenticated: (state) => !!state.token,
    userRole: (state) => state.user?.role || null,
    userId: (state) => state.user?.id || null,
    userName: (state) => state.user ? `${state.user.firstname || ''} ${state.user.secondname || ''} ${state.user.thirdname || ''}`.trim() : 'Гость',
  },
  actions: {
    async register(userData) {
      this.status = 'loading';
      this.error = null;
      try {
        const encodedData = {
          firstname: encode(userData.firstname),
          secondname: encode(userData.secondname),
          thirdname: encode(userData.thirdname),
          password: encode(userData.password),
        };
        const response = await apiClient.post('/user/register', encodedData);
        this.status = 'success';
        const message = response.data?.message ? decode(response.data.message) : 'Регистрация успешна!';
        alert(message);
        router.push('/login');
      } catch (error) {
        this.status = 'error';
         let decodedErrorMessage = 'Ошибка регистрации.';
         if (error.response?.data?.message) {
             try { decodedErrorMessage = decode(error.response.data.message); } catch (e) { console.error("Failed to decode error message", e); }
         } else if (typeof error.response?.data === 'string') {
             try { decodedErrorMessage = decode(error.response.data); } catch (e) { console.error("Failed to decode error message", e); }
         }
         this.error = decodedErrorMessage;
        console.error('Registration failed:', this.error);
      }
    },

    async login(credentials) {
      this.status = 'loading';
      this.error = null;
      try {
        const encodedCredentials = {
          firstname: encode(credentials.firstname),
          secondname: encode(credentials.secondname),
          thirdname: encode(credentials.thirdname),
          password: encode(credentials.password),
        };
        const response = await apiClient.post('/user/login', encodedCredentials);

        const base64Token = response.data.token;
        if (!base64Token) {
            throw new Error("Token not received from server.");
        }

        const actualToken = decode(base64Token);
        if (!actualToken) {
            throw new Error("Failed to decode token received from server.");
        }

        this.token = actualToken;
        localStorage.setItem('authToken', this.token);

        try {
            const decodedToken = jwtDecode(this.token);
             this.user = {
                 firstname: decodedToken.firstname,
                 secondname: decodedToken.secondname,
                 thirdname: decodedToken.thirdname,
                 role: decodedToken.role,
             };

             localStorage.setItem('authUser', JSON.stringify(this.user));

        } catch (jwtError) {
            console.error("Failed to decode JWT:", jwtError);
             this.logout();
            throw new Error("Invalid token structure received.");
        }

        this.status = 'success';
        this.error = null;

        this.navigateUser();

      } catch (error) {
        this.token = null;
        this.user = {};
        localStorage.removeItem('authToken');
        localStorage.removeItem('authUser');
        this.status = 'error';

        let decodedErrorMessage = 'Ошибка входа.';
        if (error.response?.data) {
            if (typeof error.response.data === 'string') {
                 try { decodedErrorMessage = decode(error.response.data); } catch (e) { console.error("Failed to decode error message", e); }
            } else if (error.response.data.message) {
                 try { decodedErrorMessage = decode(error.response.data.message); } catch (e) { console.error("Failed to decode error message", e); }
            }
        } else if (error.message) {
            decodedErrorMessage = error.message;
        }
        this.error = decodedErrorMessage;
        console.error('Login failed:', this.error);
      }
    },

    logout() {
      this.token = null;
      this.user = {};
      this.status = null;
      this.error = null;
      localStorage.removeItem('authToken');
      localStorage.removeItem('authUser');
      if (router.currentRoute.value.meta.requiresAuth) {
         router.push('/login');
      }
    },

     checkAuth() {
        const token = localStorage.getItem('authToken');
        const user = JSON.parse(localStorage.getItem('authUser') || '{}');

        if (token && user) {
            try {
                const decoded = jwtDecode(token);
                const now = Date.now() / 1000;
                if (decoded.exp && decoded.exp > now) {
                    this.token = token;
                    this.user = user;
                } else {
                    console.log("Stored token expired.");
                    this.logout();
                }
            } catch (e) {
                console.error("Error decoding stored token:", e);
                this.logout();
            }
        } else {
            this.logout();
        }
    },

    navigateUser() {
        if (!this.isAuthenticated) {
            router.push('/login');
            return;
        }
        switch (this.userRole) {
            case 'Admiral':
                router.push('/admiral');
                break;
            case 'Captain':
                router.push('/captain');
                break;
            case 'Sailor':
            case '':
            default:
                router.push('/dashboard');
                break;
        }
    }
  },
});