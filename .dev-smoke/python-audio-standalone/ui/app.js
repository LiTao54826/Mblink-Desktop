import { h, render } from 'preact';
import { useEffect, useRef, useState } from 'preact/hooks';

const AUDIO_SRC = '/assets/audio/mbink-audio-smoke-1khz.wav';

function mountRoot() {
  document.documentElement.style.width = '100%';
  document.documentElement.style.height = '100%';
  document.documentElement.style.margin = '0';
  document.body.style.width = '100%';
  document.body.style.height = '100%';
  document.body.style.margin = '0';
  document.body.style.overflow = 'hidden';

  let root = document.getElementById('root');
  if (!root) {
    root = document.createElement('div');
    root.id = 'root';
    document.body.appendChild(root);
  }
  root.style.width = '100%';
  root.style.height = '100%';
  return root;
}

function App() {
  const audioRef = useRef(null);
  const [status, setStatus] = useState('waiting');
  const [events, setEvents] = useState([]);

  const pushEvent = (message) => {
    setEvents((items) => [`${new Date().toLocaleTimeString()} ${message}`, ...items].slice(0, 5));
  };

  const playAudio = async () => {
    const audio = audioRef.current;
    if (!audio) return;
    audio.volume = 1;
    audio.muted = false;
    audio.loop = true;
    try {
      const result = audio.play();
      if (result && typeof result.then === 'function') await result;
      setStatus('playing');
      pushEvent('play() resolved');
    } catch (error) {
      setStatus(`play failed: ${error?.message || error}`);
      pushEvent(`play failed: ${error?.message || error}`);
    }
  };

  useEffect(() => {
    const timer = setTimeout(playAudio, 700);
    return () => clearTimeout(timer);
  }, []);

  return h('main', {
    style: {
      width: '100%',
      height: '100%',
      display: 'grid',
      gridTemplateRows: 'auto auto 1fr',
      gap: '14px',
      padding: '22px',
      boxSizing: 'border-box',
      fontFamily: 'Segoe UI, Arial, sans-serif',
      color: '#171717',
      background: '#f7f8fb'
    }
  },
    h('header', { style: { display: 'grid', gap: '4px' } },
      h('h1', { style: { margin: 0, fontSize: '22px', fontWeight: 650 } }, 'Python audio smoke'),
      h('div', { id: 'status', style: { fontSize: '13px', color: '#555' } }, status)
    ),
    h('section', {
      style: {
        display: 'grid',
        gridTemplateColumns: '1fr auto',
        gap: '12px',
        alignItems: 'center'
      }
    },
      h('audio', {
        id: 'audio',
        ref: audioRef,
        src: AUDIO_SRC,
        controls: true,
        loop: true,
        preload: 'auto',
        onCanPlay: () => pushEvent('canplay'),
        onPlay: () => {
          setStatus('playing');
          pushEvent('play event');
        },
        onPause: () => {
          setStatus('paused');
          pushEvent('pause event');
        },
        onError: () => {
          const error = audioRef.current?.error;
          setStatus(`error ${error?.code || 'unknown'}`);
          pushEvent(`error ${error?.code || 'unknown'}`);
        },
        style: { width: '100%', minWidth: 0 }
      }),
      h('button', {
        id: 'play-button',
        onClick: playAudio,
        style: {
          height: '40px',
          padding: '0 16px',
          border: '1px solid #9aa4b2',
          borderRadius: '6px',
          background: '#ffffff',
          color: '#171717',
          fontSize: '14px'
        }
      }, 'Play')
    ),
    h('section', {
      id: 'event-log',
      style: {
        minHeight: 0,
        overflow: 'auto',
        padding: '10px 12px',
        border: '1px solid #d8dde6',
        borderRadius: '6px',
        background: '#ffffff',
        fontFamily: 'Consolas, monospace',
        fontSize: '12px',
        color: '#333'
      }
    }, events.length ? events.map((item) => h('div', { key: item }, item)) : h('div', null, 'no events yet'))
  );
}

render(h(App), mountRoot());
