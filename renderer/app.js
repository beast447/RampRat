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

function updateSpeed(value) {
  window.api.sendCommand(`speed:${parseFloat(value).toFixed(2)}`);
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
  const speedSlider = document.getElementById('slider-speed');
  const speedValue = document.getElementById('speed-value');
  speedValue.textContent = `${speedSlider.value} m/s`;
  speedSlider.oninput = () => {
    speedValue.textContent = `${speedSlider.value} m/s`;
    updateSpeed(speedSlider.value);
  };
  document.getElementById('btn-left').onclick = () => {
    slider.value = -45;
    angleValue.textContent = '-45°';
    updateSteering(-45);
  };
  document.getElementById('btn-straight').onclick = () => {
    slider.value = 0;
    angleValue.textContent = '0°';
    updateSteering(0);
  };
  document.getElementById('btn-right').onclick = () => {
    slider.value = 45;
    angleValue.textContent = '45°';
    updateSteering(45);
  };
};
