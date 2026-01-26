import React from "react";
import { Text, View, DeviceEventEmitter } from "react-native";
import { multiply, udpBind, udpCreate, udpClose } from "reactive-socks";


export default function App() {
  const [last, setLast] = React.useState("none");
  const udpRef = React.useRef<number | null>(null);

  React.useEffect(() => {
    const h = udpCreate();
    udpRef.current = h;
    udpBind(h, 7573, "0.0.0.0");

    const sub = DeviceEventEmitter.addListener('udpMessage', (pkt) => {
      console.log("Packet", pkt);
      setLast(`${pkt.host}:${pkt.port} size=${pkt.data_size ?? pkt.dataSize ?? "?"}`);
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
