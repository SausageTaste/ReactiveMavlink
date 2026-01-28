import React from "react";
import { Text, View, Pressable, DeviceEventEmitter } from "react-native";
import { multiply, udpBind, udpCreate, udpClose, udpSend } from "reactive-socks";
import { MavlinkFramer } from "ma-mavlink";


export default function App() {
  const [pktCount, setPktCount] = React.useState(0);
  const [last, setLast] = React.useState("none");
  const [lastMavlinkPkt, setLastMavlinkPkt] = React.useState("none");
  const udpRef = React.useRef<number | null>(null);
  const framerRef = React.useRef<MavlinkFramer | null>(null);

  const sendPacket = React.useCallback(async () => {
    console.log("multiply:", multiply);
    console.log("udpBind:", udpBind);
    console.log("udpCreate:", udpCreate);
    console.log("udpClose:", udpClose);
    console.log("udpSend:", udpSend);

    const result = udpSend(udpRef.current, "192.168.100.10", 7573, "Invalid data");
    if (result) {
      console.log("Successfully sent!");
    }
    else {
      console.log("Failed sending");
    }
  }, []);

  React.useEffect(() => {
    const h = udpCreate();
    udpRef.current = h;

    if (!udpBind(h, 7573, "192.168.100.10")) {
      console.log("Failed to bind UDP socket");
      return;
    }

    framerRef.current = new MavlinkFramer((pkt: object) => {
      setPktCount(prevCount => prevCount + 1);
      setLastMavlinkPkt(JSON.stringify(pkt));
    });

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
      <Text>Last MavLink: {lastMavlinkPkt}</Text>
      <Text>MavLink packet count: {pktCount}</Text>
      <Text>3 x 7 = {multiply(3, 7)}</Text>
      <Text>UDP handle = {udpRef.current}</Text>
      <Pressable
        onPress={sendPacket}
        style={{
          marginTop: 16,
          paddingVertical: 10,
          paddingHorizontal: 14,
          borderWidth: 1,
          borderRadius: 6,
          alignSelf: "flex-start",
        }}
      >
        <Text>Send</Text>
      </Pressable>
    </View>
  );
}
