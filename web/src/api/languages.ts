// ISO 639-2/B codes used for default audio/subtitle language preferences (matches the
// server's audio_language/subtitle_language defaults: 'eng'/'en').
export const LANGUAGES: { code: string; label: string }[] = [
  { code: 'eng', label: 'English' },
  { code: 'spa', label: 'Spanish' },
  { code: 'fre', label: 'French' },
  { code: 'ger', label: 'German' },
  { code: 'ita', label: 'Italian' },
  { code: 'por', label: 'Portuguese' },
  { code: 'jpn', label: 'Japanese' },
  { code: 'kor', label: 'Korean' },
  { code: 'chi', label: 'Chinese' },
  { code: 'rus', label: 'Russian' },
  { code: 'hin', label: 'Hindi' },
  { code: 'ara', label: 'Arabic' },
];

// Map a track's BCP-47 / ISO code to one of our codes for matching (e.g. 'en','eng','en-US').
export function matchesLang(trackLang: string | undefined, pref: string): boolean {
  if (!trackLang || !pref) return false;
  const t = trackLang.toLowerCase();
  const p = pref.toLowerCase();
  const alias: Record<string, string[]> = {
    eng: ['en', 'eng'], spa: ['es', 'spa'], fre: ['fr', 'fre', 'fra'], ger: ['de', 'ger', 'deu'],
    ita: ['it', 'ita'], por: ['pt', 'por'], jpn: ['ja', 'jpn'], kor: ['ko', 'kor'],
    chi: ['zh', 'chi', 'zho'], rus: ['ru', 'rus'], hin: ['hi', 'hin'], ara: ['ar', 'ara'],
  };
  return (alias[p] ?? [p]).some((a) => t === a || t.startsWith(a + '-'));
}
