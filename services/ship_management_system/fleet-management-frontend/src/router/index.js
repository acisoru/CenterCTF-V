import { createRouter, createWebHistory } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const LoginView = () => import('../views/LoginView.vue')
const RegisterView = () => import('../views/RegisterView.vue')
const UserDashboardView = () => import('../views/UserDashboardView.vue')
const AdmiralDashboardView = () => import('../views/AdmiralDashboardView.vue')
const CaptainDashboardView = () => import('../views/CaptainDashboardView.vue')
const NotFoundView = () => import('../views/NotFoundView.vue')

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'home',
      redirect: () => {
        const authStore = useAuthStore();
        if (authStore.isAuthenticated) {
            switch (authStore.userRole) {
                case 'Admiral': return '/admiral';
                case 'Captain': return '/captain';
                default: return '/dashboard';
            }
        }
        return '/login';
      }
    },
    {
      path: '/login',
      name: 'login',
      component: LoginView,
      meta: { requiresGuest: true }
    },
    {
      path: '/register',
      name: 'register',
      component: RegisterView,
      meta: { requiresGuest: true }
    },
    {
      path: '/dashboard',
      name: 'user-dashboard',
      component: UserDashboardView,
      meta: { requiresAuth: true, roles: ['Sailor', 'None', null, undefined] }
    },
    {
      path: '/admiral',
      name: 'admiral-dashboard',
      component: AdmiralDashboardView,
      meta: { requiresAuth: true, roles: ['Admiral'] }
    },
    {
      path: '/captain',
      name: 'captain-dashboard',
      component: CaptainDashboardView,
      meta: { requiresAuth: true, roles: ['Captain'] }
    },
    {
      path: '/:pathMatch(.*)*',
      name: 'NotFound',
      component: NotFoundView
    }
  ]
})

router.beforeEach((to, from, next) => {
  const authStore = useAuthStore();
  const requiresAuth = to.matched.some(record => record.meta.requiresAuth);
  const requiresGuest = to.matched.some(record => record.meta.requiresGuest);
  const requiredRoles = to.meta.roles;

  if (requiresAuth && !authStore.isAuthenticated) {
    next({ name: 'login', query: { redirect: to.fullPath } });
  } else if (requiresGuest && authStore.isAuthenticated) {
     switch (authStore.userRole) {
         case 'Admiral': next({ name: 'admiral-dashboard' }); break;
         case 'Captain': next({ name: 'captain-dashboard' }); break;
         default: next({ name: 'user-dashboard' }); break;
     }
  } else if (requiresAuth && requiredRoles && !requiredRoles.includes(authStore.userRole)) {
    console.warn(`Access denied: Route ${to.path} requires roles ${requiredRoles}, user has role ${authStore.userRole}`);
     switch (authStore.userRole) {
         case 'Admiral': next({ name: 'admiral-dashboard' }); break;
         case 'Captain': next({ name: 'captain-dashboard' }); break;
         default: next({ name: 'user-dashboard' }); break;
     }
  }
  else {
    next();
  }
});


export default router