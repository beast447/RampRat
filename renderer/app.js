let isPushbackActive = false;

function togglePushback() {
  const button = document.getElementById('pushbackBtn');
  const steerDiv = document.getElementById('steering-controls');

  if (!isPushbackActive) {
    window.api.sendCommand('pushback');
    button.textContent = 'Stop Pushback';
    steerDiv.style.display = 'block';
    isPushbackActive = true;
  } else {
    window.api.sendCommand('pushback-stop');
    button.textContent = 'Start Pushback';
    steerDiv.style.display = 'none';
    isPushbackActive = false;
  }
}

function updateSteering(value) {
  const radians = (parseFloat(value) * Math.PI) / 180;
  window.api.sendCommand(`angle:${radians.toFixed(4)}`);
}
// ...existing code...

window.onload = () => {
  document.getElementById('pushbackBtn').onclick = togglePushback;
  const slider = document.getElementById('slider-angle');
  const angleValue = document.getElementById('angle-value');
  slider.oninput = (e) => {
    angleValue.textContent = `${slider.value}°`;
    updateSteering(slider.value);
  };
  document.getElementById('btn-left').onclick = () => window.api.sendCommand('state:1');
  document.getElementById('btn-straight').onclick = () => window.api.sendCommand('state:0');
  document.getElementById('btn-right').onclick = () => window.api.sendCommand('state:2');
};
