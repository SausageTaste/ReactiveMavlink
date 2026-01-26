import React from "react";
import { Text, View, NativeEventEmitter, NativeModules } from "react-native";
import { Buffer } from "buffer";
import { multiply, udpBind, udpCreate } from "reactive-socks";

export default function App() {
  const [last, setLast] = React.useState("none");

  const udp = udpCreate();
  udpBind(udp, 1, "damn");

  return (
    <View style={{ padding: 20 }}>
      <Text>Listening UDP…</Text>
      <Text>Last packet: {last}</Text>
      <Text>3 x 7 = {multiply(3, 7)}</Text>
      <Text>UDP handle = {udpCreate()}</Text>
    </View>
  );
}
