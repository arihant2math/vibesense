import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.concurrent.atomic.AtomicBoolean;

public class HeartbeatDemo {
    private static final int PORT = 5000;
    private static final String HEARTBEAT = "HEARTBEAT";
    private static final String ACK = "ACK";

    public static void main(String[] args) throws Exception {
        HeartbeatReceiver receiver = new HeartbeatReceiver(PORT);
        HeartbeatSender sender = new HeartbeatSender(
                InetAddress.getLoopbackAddress(), PORT, 1000
        );

        receiver.start();
        Thread.sleep(200);
        sender.start();

        Thread.sleep(5000);

        sender.stop();
        receiver.stop();
        sender.join();
        receiver.join();
    }

    static class HeartbeatSender extends Thread {
        private final InetAddress address;
        private final int port;
        private final long intervalMillis;
        private final AtomicBoolean running = new AtomicBoolean(true);
        private DatagramSocket socket;

        HeartbeatSender(InetAddress address, int port, long intervalMillis) {
            this.address = address;
            this.port = port;
            this.intervalMillis = intervalMillis;
        }

        @Override
        public void run() {
            try (DatagramSocket datagramSocket = new DatagramSocket()) {
                socket = datagramSocket;
                socket.setSoTimeout((int) intervalMillis);

                byte[] heartbeat = HEARTBEAT.getBytes(StandardCharsets.UTF_8);

                while (running.get()) {
                    DatagramPacket packet = new DatagramPacket(
                            heartbeat, heartbeat.length, address, port
                    );
                    socket.send(packet);
                    System.out.println("Sent heartbeat at " + Instant.now());

                    byte[] buffer = new byte[64];
                    DatagramPacket response = new DatagramPacket(buffer, buffer.length);

                    try {
                        socket.receive(response);
                        String message = new String(
                                response.getData(),
                                response.getOffset(),
                                response.getLength(),
                                StandardCharsets.UTF_8
                        );
                        System.out.println("Received: " + message);
                    } catch (java.net.SocketTimeoutException e) {
                        System.out.println("Heartbeat acknowledgment timed out");
                    }

                    Thread.sleep(intervalMillis);
                }
            } catch (IOException e) {
                if (running.get()) {
                    e.printStackTrace();
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }

        void stop() {
            running.set(false);
            if (socket != null) {
                socket.close();
            }
            interrupt();
        }
    }

    static class HeartbeatReceiver extends Thread {
        private final int port;
        private final AtomicBoolean running = new AtomicBoolean(true);
        private DatagramSocket socket;

        HeartbeatReceiver(int port) {
            this.port = port;
        }

        @Override
        public void run() {
            try (DatagramSocket datagramSocket = new DatagramSocket(port)) {
                socket = datagramSocket;
                byte[] buffer = new byte[128];

                while (running.get()) {
                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                    socket.receive(packet);

                    String message = new String(
                            packet.getData(),
                            packet.getOffset(),
                            packet.getLength(),
                            StandardCharsets.UTF_8
                    );

                    if (HEARTBEAT.equals(message)) {
                        System.out.println("Received heartbeat from "
                                + packet.getAddress().getHostAddress()
                                + ":" + packet.getPort());

                        byte[] acknowledgment = ACK.getBytes(StandardCharsets.UTF_8);
                        DatagramPacket response = new DatagramPacket(
                                acknowledgment,
                                acknowledgment.length,
                                packet.getAddress(),
                                packet.getPort()
                        );
                        socket.send(response);
                    }
                }
            } catch (IOException e) {
                if (running.get()) {
                    e.printStackTrace();
                }
            }
        }

        void stop() {
            running.set(false);
            if (socket != null) {
                socket.close();
            }
            interrupt();
        }
    }
}
