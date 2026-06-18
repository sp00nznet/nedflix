import { api } from './http';
import type { InfoItem } from '../state/info';

export type MyListItem = InfoItem;

export const getMyList = () => api.get<MyListItem[]>('/api/mylist').catch(() => [] as MyListItem[]);
export const addToMyList = (item: MyListItem) => api.post<MyListItem[]>('/api/mylist', item).catch(() => [] as MyListItem[]);
export const removeFromMyList = (id: string) => api.del<MyListItem[]>(`/api/mylist?id=${encodeURIComponent(id)}`).catch(() => [] as MyListItem[]);
