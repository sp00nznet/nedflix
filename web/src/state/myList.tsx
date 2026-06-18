import { createContext, useCallback, useContext, useEffect, useMemo, useState, type ReactNode } from 'react';
import { getMyList, addToMyList, removeFromMyList, type MyListItem } from '../api/mylist';

interface MyListCtx {
  list: MyListItem[];
  has: (id: string) => boolean;
  toggle: (item: MyListItem) => void;
}
const Ctx = createContext<MyListCtx>({ list: [], has: () => false, toggle: () => {} });

export const useMyList = () => useContext(Ctx);

export function MyListProvider({ children }: { children: ReactNode }) {
  const [list, setList] = useState<MyListItem[]>([]);
  useEffect(() => { getMyList().then(setList); }, []);

  const has = useCallback((id: string) => list.some((x) => x.id === id), [list]);
  const toggle = useCallback((item: MyListItem) => {
    setList((cur) => (cur.some((x) => x.id === item.id) ? cur.filter((x) => x.id !== item.id) : [item, ...cur])); // optimistic
    (list.some((x) => x.id === item.id) ? removeFromMyList(item.id) : addToMyList(item)).then(setList);
  }, [list]);

  const value = useMemo(() => ({ list, has, toggle }), [list, has, toggle]);
  return <Ctx.Provider value={value}>{children}</Ctx.Provider>;
}
