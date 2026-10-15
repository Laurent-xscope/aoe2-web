export class AudioManager {
  private ctx: AudioContext | null = null;
  private soundBuffers: Map<string, AudioBuffer> = new Map();
  private isMuted = false;

  constructor() {
    // AudioContext will be initialized on first user interaction
  }

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
    const sounds = ['5003.wav', '5011.wav', '5012.wav', '5031.wav', '5032.wav', '5123.wav', '5127.wav'];
    for (const name of sounds) {
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
    if (this.soundBuffers.has('5003.wav')) {
      this.playBuffer(this.soundBuffers.get('5003.wav')!);
    } else {
      this.synthBeep(520, 0.08, 'triangle');
    }
  }

  public playCommand() {
    this.initContext();
    if (this.soundBuffers.has('5011.wav')) {
      this.playBuffer(this.soundBuffers.get('5011.wav')!);
    } else {
      this.synthBeep(440, 0.12, 'sine');
    }
  }

  public playAttack() {
    this.initContext();
    if (this.soundBuffers.has('5031.wav')) {
      this.playBuffer(this.soundBuffers.get('5031.wav')!);
    } else {
      this.synthNoise(0.15);
    }
  }

  public playGather() {
    this.initContext();
    if (this.soundBuffers.has('5012.wav')) {
      this.playBuffer(this.soundBuffers.get('5012.wav')!);
    } else {
      this.synthThud(120, 0.1);
    }
  }

  public playHorn() {
    this.initContext();
    if (this.soundBuffers.has('5123.wav')) {
      this.playBuffer(this.soundBuffers.get('5123.wav')!);
    } else {
      this.synthHorn();
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
