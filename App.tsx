import React from "react";
import { Text, View, DeviceEventEmitter } from "react-native";
import { MavLinkPacketSplitter, MavLinkPacketParser } from "node-mavlink";


export default function App() {
  const [last, setLast] = React.useState("none");
  const [lastMavlinkPkt, setLastMavlinkPkt] = React.useState("none");
  const splitterRef = React.useRef<MavLinkPacketSplitter | null>(null);
  const parserRef = React.useRef<MavLinkPacketParser | null>(null);

  React.useEffect(() => {
    splitterRef.current = new MavLinkPacketSplitter();
    parserRef.current = new MavLinkPacketParser();
    splitterRef.current.on("packet", packet => {
      parserRef.current?.parse(packet);
    });

    parserRef.current.on("message", message => {
      console.log('Got MAVLink message:', message);
      console.log('Message name:', message.name);
      console.log('Fields:', message.fields);
    })

    const sub = DeviceEventEmitter.addListener('udpMessage', (pkt) => {
      setLast(JSON.stringify(pkt));

      const bytes = Uint8Array.from(pkt.data);
      splitterRef.current?.write(bytes);
    });

    return () => {
      sub.remove();
    };
  }, []);

  return (
    <View style={{ padding: 20 }}>
      <Text>Listening UDP…</Text>
      <Text>Last packet: {last}</Text>
      <Text>Last MavLink: {lastMavlinkPkt}</Text>
    </View>
  );
}
