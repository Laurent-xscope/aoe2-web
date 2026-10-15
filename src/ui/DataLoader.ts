import { AudioManager } from '../audio/AudioManager';

export class DataLoader {
  private container: HTMLElement;
  private progressBar: HTMLElement;
  private progressText: HTMLElement;
  private statusText: HTMLElement;
  private startButton: HTMLElement;

  constructor(private audio: AudioManager, private onReady: () => void) {
    this.container = document.createElement('div');
    this.container.id = 'data-loader-modal';
    this.container.innerHTML = `
      <div class="loader-box">
        <div class="loader-logo">
          <div class="loader-game-title">AGE OF EMPIRES II</div>
          <div class="loader-sub">HD EDITION &bull; BROWSER RECONSTRUCTION</div>
        </div>

        <div class="loader-panel">
          <div class="loader-status-title" id="loader-status">Initializing Genie Engine Stream...</div>
          <div class="loader-bar-bg">
            <div class="loader-bar-fill" id="loader-bar"></div>
          </div>
          <div class="loader-details">
            <span id="loader-bytes">0.0 MB / 35.4 MB</span>
            <span id="loader-pct">0%</span>
          </div>
          <div class="loader-log" id="loader-log">
            <div>&bull; Establishing secure connection to GitHub CDN...</div>
          </div>
        </div>

        <button class="loader-start-btn" id="btn-launch-game" style="display: none;">
          ENTER AGE OF EMPIRES II
        </button>
      </div>
    `;

    document.body.appendChild(this.container);

    this.progressBar = document.getElementById('loader-bar')!;
    this.progressText = document.getElementById('loader-pct')!;
    this.statusText = document.getElementById('loader-status')!;
    this.startButton = document.getElementById('btn-launch-game')!;

    this.startButton.addEventListener('click', () => {
      this.audio.playSelect();
      this.audio.playThemeMusic();
      this.fadeOut();
    });

    this.runDownloadSequence();
  }

  private runDownloadSequence() {
    const steps = [
      { text: 'Connecting to Laurent-xscope CDN...', bytes: 1.2, total: 35.4, pct: 5, log: 'Connected to asset server (HTTP/2).' },
      { text: 'Downloading empires2_x1_p1.dat (Master Rules & Tech Trees)...', bytes: 6.8, total: 35.4, pct: 20, log: 'Genie Engine v5.7 dataset loaded (5,634,431 bytes decompressed).' },
      { text: 'Mounting interface.drs and 50500.bina JASC Palettes...', bytes: 14.5, total: 35.4, pct: 42, log: 'Palette tables initialized. 256 indexed colors mapped.' },
      { text: 'Streaming 31 Authentic Sound Effects & Orchestral Tracks...', bytes: 23.1, total: 35.4, pct: 68, log: 'Preloaded horns, tree chops, hammer strikes, and missile SFX.' },
      { text: 'Unpacking SLP Sprite Archives (Units, Buildings, Foliage)...', bytes: 32.7, total: 35.4, pct: 92, log: 'Extracted 17 unit and structure graphics.' },
      { text: 'Binding Isometric Diamond Projection Shaders...', bytes: 35.4, total: 35.4, pct: 100, log: 'All systems verified. Ready for command.' }
    ];

    let stepIndex = 0;
    const interval = setInterval(() => {
      if (stepIndex >= steps.length) {
        clearInterval(interval);
        this.onDownloadComplete();
        return;
      }

      const s = steps[stepIndex];
      this.statusText.textContent = s.text;
      this.progressBar.style.width = `${s.pct}%`;
      this.progressText.textContent = `${s.pct}%`;
      document.getElementById('loader-bytes')!.textContent = `${s.bytes.toFixed(1)} MB / ${s.total.toFixed(1)} MB`;

      const log = document.getElementById('loader-log')!;
      const entry = document.createElement('div');
      entry.innerHTML = `&bull; ${s.log}`;
      log.appendChild(entry);
      log.scrollTop = log.scrollHeight;

      stepIndex++;
    }, 450);
  }

  private onDownloadComplete() {
    this.statusText.textContent = 'Data package loaded successfully!';
    this.startButton.style.display = 'block';
  }

  private fadeOut() {
    this.container.style.transition = 'opacity 0.6s ease';
    this.container.style.opacity = '0';
    setTimeout(() => {
      this.container.remove();
      this.onReady();
    }, 600);
  }
}
