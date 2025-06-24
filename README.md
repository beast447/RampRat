# GSX Lite

GSX Lite is a proof of concept ground service tool for Microsoft Flight Simulator 2024. It is built with [Electron](https://electronjs.org/) for the user interface and uses a small C++ bridge to communicate with the simulator through the SimConnect SDK.

## Requirements

- **Node.js** v18 or later with `npm`
- **Electron** `^28.0.0` (installed as a dev dependency)
- Windows with MSFS 2024 installed
- (optional) the MSFS SDK if you want to rebuild the SimConnect bridge

## Setup

1. Install dependencies:
   ```bash
   npm install
   ```
2. Start the application:
   ```bash
   npm start
   ```
   The Electron main process automatically spawns `simconnect-bridge.exe` and communicates with it over standard input/output.

## simconnect-bridge.exe

The bridge is a console application that receives simple text commands and forwards them to MSFS using SimConnect. The Electron app launches it using Node's `spawn` API, as shown in `main.js`:

```javascript
app.whenReady().then(() => {
  bridgeProcess = spawn(path.join(__dirname, 'simconnect-bridge.exe'));
  // ...read and write to the process...
});
```

The bridge reads lines like `pushback`, `pushback-stop`, `angle:<radians>` and
`speed:<meters per second>` from its standard input. When `pushback` is
received, it freezes the aircraft and repeatedly repositions it to simulate a
tug. `angle:` adjusts the steering angle while the loop runs, `speed:` controls
how fast the aircraft moves (meters per second) during the pushback, and
`pushback-stop` ends the sequence and unfreezes the plane. You can also send
`baggage-start` and `baggage-stop` to trigger the baggage trucks. The
implementation lives in `main.cpp`.

## Building the bridge from source

If you need to rebuild `simconnect-bridge.exe`, compile `main.cpp` with the SimConnect headers and library from the MSFS SDK. A typical command from a Visual Studio Developer Command Prompt might look like:

```cmd
cl main.cpp /EHsc /I "C:\\MSFS 2024 SDK\\SimConnect SDK\\include" \
   /link /LIBPATH:"C:\\MSFS 2024 SDK\\SimConnect SDK\\lib" SimConnect.lib \
   /OUT:simconnect-bridge.exe
```

Ensure `SimConnect.dll` is available in the application directory or on your system `PATH` when running the Electron app.
