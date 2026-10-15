export class AudioManager {
  private ctx: AudioContext | null = null;
  private soundBuffers: Map<string, AudioBuffer> = new Map();
  private isMuted = false;

  constructor() {}

  private initContext() {
    if (!this.ctx) {
      const AudioCtx = window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
      this.ctx = new AudioCtx();
    }
    if (this.ctx.state === 'suspended') {
      this.ctx.resume();
    }
  }

  public async preloadSounds() {
    this.initContext();
    const soundList = [
      '5003.wav', '5011.wav', '5012.wav', '5031.wav', '5032.wav', '5043.wav', '5045.wav',
      '5100.wav', '5110.wav', '5123.wav', '5127.wav', '5134.wav', '5229.wav', '5272.wav',
      '5276.wav', '5277.wav', '5286.wav', '5287.wav', '5288.wav', '5289.wav', '5301.wav',
      '5302.wav', '5303.wav', '5332.wav', '5333.wav', '5334.wav', '5358.wav', '5359.wav',
      '5360.wav', '5361.wav', '5412.wav', '5498.wav', '5499.wav'
    ];

    for (const name of soundList) {
      try {
        const res = await fetch(`./assets/sounds/${name}`);
        if (res.ok) {
          const arrayBuffer = await res.arrayBuffer();
          if (this.ctx) {
            const audioBuffer = await this.ctx.decodeAudioData(arrayBuffer);
            this.soundBuffers.set(name, audioBuffer);
          }
        }
      } catch {
        // Fallback procedural sounds will be used
      }
    }
  }

  public playSelect() {
    this.initContext();
    this.playOneOf(['5003.wav'], () => this.synthBeep(520, 0.08, 'triangle'));
  }

  public playCommand() {
    this.initContext();
    this.playOneOf(['5011.wav'], () => this.synthBeep(440, 0.12, 'sine'));
  }

  public playAttack() {
    this.playAttackMelee();
  }

  public playAttackMelee() {
    this.initContext();
    this.playOneOf(['5358.wav', '5359.wav', '5360.wav', '5361.wav'], () => this.synthNoise(0.12));
  }

  public playAttackRanged() {
    this.initContext();
    this.playOneOf(['5286.wav', '5287.wav', '5288.wav', '5289.wav'], () => this.synthBeep(880, 0.08, 'sawtooth'));
  }

  public playGather() {
    this.playChop();
  }

  public playChop() {
    this.initContext();
    this.playOneOf(['5301.wav', '5302.wav', '5303.wav'], () => this.synthThud(120, 0.1));
  }

  public playHammer() {
    this.initContext();
    this.playOneOf(['5332.wav', '5333.wav', '5334.wav'], () => this.synthThud(160, 0.08));
  }

  public playBuildingHit() {
    this.initContext();
    this.playOneOf(['5412.wav', '5498.wav', '5499.wav'], () => this.synthNoise(0.2));
  }

  public playHorn() {
    this.initContext();
    this.playOneOf(['5123.wav'], () => this.synthHorn());
  }

  public playTownBell() {
    this.initContext();
    // Play town center bell alarm
    if (this.soundBuffers.has('5123.wav')) {
      this.playBuffer(this.soundBuffers.get('5123.wav')!);
    } else {
      this.synthBeep(659.25, 0.5, 'sine');
    }
  }

  public playBuildingComplete(typeSound?: string) {
    this.initContext();
    if (typeSound && this.soundBuffers.has(typeSound)) {
      this.playBuffer(this.soundBuffers.get(typeSound)!);
    } else {
      this.playHorn();
    }
  }

  private musicAudio: HTMLAudioElement | null = null;

  public playThemeMusic() {
    try {
      if (!this.musicAudio) {
        this.musicAudio = new Audio('./assets/sounds/theme.mp3');
        this.musicAudio.loop = true;
        this.musicAudio.volume = 0.55;
      }
      this.musicAudio.play().catch(() => {});
    } catch {
      // autoplay restriction handled on user interaction
    }
  }

  public stopThemeMusic() {
    if (this.musicAudio) {
      this.musicAudio.pause();
      this.musicAudio.currentTime = 0;
    }
  }

  public playCivJingle(civ = 'british') {
    try {
      const file = civ === 'british' ? './assets/sounds/civ_british.mp3' : './assets/sounds/civ_french.mp3';
      const a = new Audio(file);
      a.volume = 0.7;
      a.play().catch(() => {});
    } catch {}
  }

  private playOneOf(names: string[], fallback: () => void) {
    const available = names.filter((n) => this.soundBuffers.has(n));
    if (available.length > 0) {
      const choice = available[Math.floor(Math.random() * available.length)];
      this.playBuffer(this.soundBuffers.get(choice)!);
    } else {
      fallback();
    }
  }

  private playBuffer(buf: AudioBuffer) {
    if (!this.ctx || this.isMuted) return;
    const src = this.ctx.createBufferSource();
    src.buffer = buf;
    src.connect(this.ctx.destination);
    src.start();
  }

  private synthBeep(freq: number, duration: number, type: OscillatorType = 'sine') {
    if (!this.ctx || this.isMuted) return;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();
    osc.type = type;
    osc.frequency.setValueAtTime(freq, this.ctx.currentTime);
    gain.gain.setValueAtTime(0.2, this.ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + duration);
    osc.connect(gain);
    gain.connect(this.ctx.destination);
    osc.start();
    osc.stop(this.ctx.currentTime + duration);
  }

  private synthThud(freq: number, duration: number) {
    if (!this.ctx || this.isMuted) return;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();
    osc.type = 'triangle';
    osc.frequency.setValueAtTime(freq, this.ctx.currentTime);
    osc.frequency.exponentialRampToValueAtTime(30, this.ctx.currentTime + duration);
    gain.gain.setValueAtTime(0.3, this.ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + duration);
    osc.connect(gain);
    gain.connect(this.ctx.destination);
    osc.start();
    osc.stop(this.ctx.currentTime + duration);
  }

  private synthNoise(duration: number) {
    if (!this.ctx || this.isMuted) return;
    const bufferSize = this.ctx.sampleRate * duration;
    const buffer = this.ctx.createBuffer(1, bufferSize, this.ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < bufferSize; i++) {
      data[i] = Math.random() * 2 - 1;
    }
    const noise = this.ctx.createBufferSource();
    noise.buffer = buffer;
    const filter = this.ctx.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = 1000;
    const gain = this.ctx.createGain();
    gain.gain.setValueAtTime(0.2, this.ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + duration);
    noise.connect(filter);
    filter.connect(gain);
    gain.connect(this.ctx.destination);
    noise.start();
  }

  private synthHorn() {
    if (!this.ctx || this.isMuted) return;
    const osc1 = this.ctx.createOscillator();
    const osc2 = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc1.type = 'sawtooth';
    osc2.type = 'sawtooth';
    osc1.frequency.setValueAtTime(220, this.ctx.currentTime);
    osc2.frequency.setValueAtTime(277.18, this.ctx.currentTime); // C# chord

    gain.gain.setValueAtTime(0.15, this.ctx.currentTime);
    gain.gain.linearRampToValueAtTime(0.25, this.ctx.currentTime + 0.2);
    gain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + 1.2);

    osc1.connect(gain);
    osc2.connect(gain);
    gain.connect(this.ctx.destination);

    osc1.start();
    osc2.start();
    osc1.stop(this.ctx.currentTime + 1.2);
    osc2.stop(this.ctx.currentTime + 1.2);
  }
}
