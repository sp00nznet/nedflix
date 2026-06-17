// Path<->id codec matching marquee-service.js (base64url of the UTF-8 file path).
export function encodePathId(path: string): string {
  const b64 = btoa(unescape(encodeURIComponent(path)));
  return b64.replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

export function decodePathId(id: string): string {
  let b64 = id.replace(/-/g, '+').replace(/_/g, '/');
  while (b64.length % 4) b64 += '=';
  return decodeURIComponent(escape(atob(b64)));
}
