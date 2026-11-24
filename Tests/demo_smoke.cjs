const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');

const html = fs.readFileSync(path.join(__dirname, '..', 'demo', 'AureaPiano-demo.html'), 'utf8');
const script = html.split('<script>')[1].split('</script>')[0];
assert(script, 'demo script exists');

const elements = new Map();
class Element {
  constructor(id = '', tagName = 'DIV') {
    this.id = id; this.tagName = tagName; this.value = '';
    this.checked = false; this.disabled = false; this.children = []; this.dataset = {}; this.style = {}; this.attributes = {};
    this.listeners = new Map(); this.textContent = ''; this.output = {textContent:''};
    this.classList = {add(){},remove(){},toggle(){}};
  }
  addEventListener(name, callback) { this.listeners.set(name, callback); }
  dispatchEvent(event) { this.listeners.get(event.type)?.(event); }
  appendChild(child) { this.children.push(child); }
  closest() { return {querySelector: () => this.output}; }
  setPointerCapture() {}
  focus() { document.activeElement = this; }
  setAttribute(name, value) { this.attributes[name] = value; }
}
for (const match of html.matchAll(/<([a-z][a-z0-9]*)[^>]*\bid="([^"]+)"[^>]*>/g)) {
  const el = new Element(match[2], match[1].toUpperCase());
  const initial = match[0].match(/\bvalue="([^"]+)"/);
  if (initial) el.value = initial[1];
  elements.set(el.id, el);
}
elements.get('aZone').value = 'all';
elements.get('bZone').value = 'off';
elements.get('bTimbre').value = 'piano';

let scriptNode;
const audioNode = () => ({connect(){}, gain:{value:0}, threshold:{value:0}, ratio:{value:1},
  attack:{value:0}, release:{value:0}, knee:{value:0}, frequency:{value:0},
  Q:{value:0}, delayTime:{value:0}, buffer:null});
class FakeAudioContext {
  constructor() { this.sampleRate = 48000; this.destination = audioNode(); }
  createScriptProcessor() { scriptNode = audioNode(); return scriptNode; }
  createBiquadFilter() { return audioNode(); }
  createDynamicsCompressor() { return audioNode(); }
  createGain() { return audioNode(); }
  createDelay() { return audioNode(); }
  createConvolver() { return audioNode(); }
  createBuffer(channels, length) {
    const data = Array.from({length:channels}, () => new Float32Array(length));
    return {getChannelData: channel => data[channel]};
  }
  resume() { return Promise.resolve(); }
}
const document = {
  activeElement:null,
  listeners:new Map(),
  getElementById:id => elements.get(id),
  createElement:tag => new Element('', tag.toUpperCase()),
  addEventListener(name, callback) { this.listeners.set(name, callback); }
};
const window = {AudioContext:FakeAudioContext, addEventListener(){}};
const context = {document, window, navigator:{}, Event:class {constructor(type){this.type=type;}},
  setTimeout, clearTimeout, Math, Float32Array};
vm.runInNewContext(script, context, {filename:'AureaPiano-demo.html'});

(async () => {
  await elements.get('start').listeners.get('click')();
  assert(scriptNode?.onaudioprocess, 'audio callback is connected');
  const firstKey = elements.get('keys').children[0];
  assert(firstKey, 'keyboard was drawn');
  firstKey.listeners.get('pointerdown')({preventDefault(){},pointerId:1,pointerType:'mouse'});
  const left = new Float32Array(512), right = new Float32Array(512);
  scriptNode.onaudioprocess({outputBuffer:{getChannelData:ch => ch ? right : left}});
  const energy = left.reduce((sum, sample) => sum + sample * sample, 0)
               + right.reduce((sum, sample) => sum + sample * sample, 0);
  assert(energy > 0.00001, 'pressing a key generates audio');
  firstKey.listeners.get('pointerup')({pointerId:1});
  elements.get('panic').listeners.get('click')();
  elements.get('aZone').value = 'right';
  elements.get('bZone').value = 'left';
  elements.get('bTimbre').value = 'bass';
  elements.get('octDown').listeners.get('click')();
  const bassKey = elements.get('keys').children[0];
  bassKey.listeners.get('pointerdown')({preventDefault(){},pointerId:2,pointerType:'mouse'});
  left.fill(0); right.fill(0);
  scriptNode.onaudioprocess({outputBuffer:{getChannelData:ch => ch ? right : left}});
  const bassEnergy = left.reduce((sum, sample) => sum + sample * sample, 0);
  assert(bassEnergy > 0.00001, 'split bass layer generates audio');
  elements.get('eqPreset').value = 'bright';
  elements.get('eqPreset').listeners.get('change')({target:elements.get('eqPreset')});
  assert.equal(Number(elements.get('eqHigh').value), 3, 'EQ preset changes bands');
  elements.get('dryTest').listeners.get('click')();
  assert.equal(elements.get('dryTest').attributes['aria-pressed'], 'true');
  assert.equal(elements.get('sustainHold').disabled, true, 'dry test disables sustain');
  console.log('Demo smoke test: keyboard, split and audio OK');
})().catch(error => { console.error(error); process.exitCode = 1; });
