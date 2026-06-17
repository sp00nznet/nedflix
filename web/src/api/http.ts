// Same-origin fetch wrapper. The web app is served by the nedflix Express server, so
// API + auth ride the existing passport session cookie (credentials: 'include').
// In dev, Vite proxies /api + /auth to the backend (see vite.config.ts).
// In a Capacitor shell the base URL points at the chosen remote server (set API_BASE).

export const API_BASE = (import.meta.env.VITE_API_BASE as string | undefined) ?? '';

export class ApiError extends Error {
  status: number;
  constructor(status: number, message: string) {
    super(message);
    this.status = status;
  }
}

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const res = await fetch(API_BASE + path, {
    credentials: 'include',
    headers: { Accept: 'application/json', ...(init?.body ? { 'Content-Type': 'application/json' } : {}), ...(init?.headers || {}) },
    ...init,
  });
  if (res.status === 401) {
    // session expired / not logged in — hand back to the server-rendered login.
    if (!API_BASE) window.location.href = '/login.html';
    throw new ApiError(401, 'Unauthorized');
  }
  if (!res.ok) throw new ApiError(res.status, `${res.status} ${res.statusText}`);
  const ct = res.headers.get('content-type') || '';
  // The nedflix API is JSON-only. A non-JSON 200 means the route wasn't handled (e.g. a
  // host serving the SPA index for unknown /api paths) — treat it as a miss so callers
  // can fall back rather than parsing HTML.
  if (!ct.includes('application/json')) throw new ApiError(res.status, 'Non-JSON response');
  return (await res.json()) as T;
}

export const api = {
  get: <T>(path: string) => request<T>(path),
  post: <T>(path: string, body?: unknown) => request<T>(path, { method: 'POST', body: body ? JSON.stringify(body) : undefined }),
  put: <T>(path: string, body?: unknown) => request<T>(path, { method: 'PUT', body: body ? JSON.stringify(body) : undefined }),
  del: <T>(path: string) => request<T>(path, { method: 'DELETE' }),
};
