import React from "react";
import { Text, View } from "react-native";
import { dgram } from "react-native-udp"


export default function App() {
  return (
    <View style={{ padding: 20 }}>
      <Text>Ready: {isReady ? "Yes" : "No"}</Text>
      <Text>Dgram handle is "{dgram ? "available" : "not available"}"</Text>
    </View>
  );
}
