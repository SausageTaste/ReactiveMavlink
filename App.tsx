import React from "react";
import { Text, View, DeviceEventEmitter } from "react-native";
import { multiply, udpBind, udpCreate, udpClose } from "reactive-socks";
import { MavlinkFramer } from "ma-mavlink";


export default function App() {
  const [last, setLast] = React.useState("none");
  const udpRef = React.useRef<number | null>(null);
  const framerRef = React.useRef<MavlinkFramer | null>(null);

  React.useEffect(() => {
    const h = udpCreate();
    udpRef.current = h;
    udpBind(h, 7573, "0.0.0.0");

    framerRef.current = new MavlinkFramer();

    const sub = DeviceEventEmitter.addListener('udpMessage', (pkt) => {
      setLast(JSON.stringify(pkt));

      const bytes = Uint8Array.from(pkt.data);
      framerRef.current.pushBytes(bytes);
      framerRef.current.process();
    });

    return () => {
      sub.remove();
      if (udpRef.current != null) {
        udpClose(udpRef.current);
        udpRef.current = null;
      }
    };
  }, []);

  return (
    <View style={{ padding: 20 }}>
      <Text>Listening UDP…</Text>
      <Text>Last packet: {last}</Text>
      <Text>3 x 7 = {multiply(3, 7)}</Text>
      <Text>UDP handle = {udpRef.current}</Text>
    </View>
  );
}
