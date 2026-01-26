import React from "react";
import { Text, View, NativeEventEmitter, NativeModules } from "react-native";
import { Buffer } from "buffer";
import { multiply, udpCreate } from "reactive-socks";

const { UdpSocket } = NativeModules;
const emitter = new NativeEventEmitter(UdpSocket);

export default function App() {
  const [last, setLast] = React.useState("none");

  React.useEffect(() => {
    const sub = emitter.addListener("onMessage", (msg: any) => {
      const bytes = new Uint8Array(Buffer.from(msg.dataBase64, "base64"));
      console.log(`UDP ${msg.host}:${msg.port} (${bytes.length} bytes)`);
      setLast(`${msg.host}:${msg.port} (${bytes.length} bytes)`);
    });

    (async () => {
      try {
        await UdpSocket.bind(7573);
        console.log("UDP bound on 7573");

        // IMPORTANT: tell native we're ready AFTER listener is added
        UdpSocket.setJsReady(true);
      } catch (e) {
        console.error("UDP init failed:", e);
      }
    })();

    return () => {
      try { UdpSocket.setJsReady(false); } catch {}
      sub.remove();
      UdpSocket?.close?.();
    };
  }, []);

  return (
    <View style={{ padding: 20 }}>
      <Text>Listening UDP…</Text>
      <Text>Last packet: {last}</Text>
      <Text>3 x 7 = {multiply(3, 7)}</Text>
      <Text>UDP handle = {udpCreate()}</Text>
    </View>
  );
}
