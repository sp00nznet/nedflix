import { Routes, Route } from 'react-router-dom';
import { SpatialFocusProvider } from './focus/SpatialFocusProvider';
import { ActiveProfileProvider } from './state/activeProfile';
import { AudioPlayerProvider } from './state/audioPlayer';
import { AppShell } from './shell/AppShell';
import { Home } from './screens/Home/Home';
import { BrowseGrid } from './screens/Browse/BrowseGrid';
import { TitleDetail } from './screens/Detail/TitleDetail';
import { VideoPlayer } from './screens/Player/VideoPlayer';
import { Music } from './screens/Music/Music';
import { Audiobooks } from './screens/Audiobooks/Audiobooks';
import { LiveTV } from './screens/Live/LiveTV';
import { Settings } from './screens/Settings/Settings';
import { Profiles } from './screens/Profiles/Profiles';
import { Search } from './screens/Search/Search';
import { Placeholder } from './screens/Placeholder';

// Routes → surfaces (see design/COMPONENT_STRUCTURE.md). Screens fill in over Phases 2–5.
export default function App() {
  return (
    <ActiveProfileProvider>
      <AudioPlayerProvider>
      <SpatialFocusProvider>
        <Routes>
          <Route element={<AppShell />}>
            <Route path="/" element={<Home />} />
            <Route path="/films" element={<BrowseGrid type="film" />} />
            <Route path="/series" element={<BrowseGrid type="series" />} />
            <Route path="/title/:id" element={<TitleDetail />} />
            <Route path="/watch/:id" element={<VideoPlayer />} />
            <Route path="/music" element={<Music />} />
            <Route path="/audiobooks" element={<Audiobooks />} />
            <Route path="/live" element={<LiveTV />} />
            <Route path="/search" element={<Search />} />
            <Route path="/settings" element={<Settings />} />
            <Route path="/profiles" element={<Profiles />} />
            <Route path="*" element={<Placeholder title="Not found" note="No surface at this route." />} />
          </Route>
        </Routes>
      </SpatialFocusProvider>
      </AudioPlayerProvider>
    </ActiveProfileProvider>
  );
}
