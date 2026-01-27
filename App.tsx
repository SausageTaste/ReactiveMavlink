import React from "react";
import { Text, View } from "react-native";
import dgram from 'react-native-udp'

export default function App() {
  const sockRef = React.useRef<any | null>(null);

  React.useEffect(() => {
    const socket = dgram.createSocket({ type: 'udp4', debug: true })
    sockRef.current = socket;
    socket.bind(12345)

    return () => {
      socket.close();
    }
  }, []);

  return (
    <View style={{ padding: 20 }}>
      <Text>Dgram handle is {JSON.stringify(sockRef.current)}</Text>
    </View>
  );
}
