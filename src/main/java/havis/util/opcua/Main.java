package havis.util.opcua;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Random;

public class Main {

	private static class Counter {
		int counter = 0;;
	}

	private static HashMap<String, Object> randomResult() {

		HashMap<String, Object> res = new HashMap<String, Object>();

		HashMap<String, Object> data = new HashMap<String, Object>();

		HashMap<String, Object> sig1 = new HashMap<String, Object>();
		HashMap<String, Object> sig2 = new HashMap<String, Object>();

		sig1.put("Antenna", new Integer((int) (Math.random() + 1.5)));
		sig1.put("CurrentPowerLevel", new Integer((int) ((Math.random() * 27) + 0.5)));
		sig1.put("Timestamp", new Long(new Date().getTime()));
		sig1.put("Strength", new Integer((int) ((-Math.random() * 64) - 0.5)));

		sig2.put("Antenna", new Integer((int) (Math.random() + 1.5)));
		sig2.put("CurrentPowerLevel", new Integer((int) ((Math.random() * 27) + 0.5)));
		sig2.put("Timestamp", new Long(new Date().getTime()));
		sig2.put("Strength", new Integer((int) ((-Math.random() * 64) - 0.5)));

		Object[] sightings = new Object[2];

		sightings[0] = sig1;
		sightings[1] = sig2;

		data.put("Sighting", sightings);

		HashMap<String, Object> scanData = new HashMap<String, Object>();

		// scanData.put("ByteString", new Byte[] {
		// (byte)((Math.random()*255)+0.5),
		// (byte)((Math.random()*255)+0.5),
		// (byte)((Math.random()*255)+0.5),
		// (byte)((Math.random()*255)+0.5)
		// });

		HashMap<String, Object> epc = new HashMap<String, Object>();
		epc.put("UId", new Byte[] { (byte) ((Math.random() * 255) + 0.5), (byte) ((Math.random() * 255) + 0.5), (byte) ((Math.random() * 255) + 0.5),
				(byte) ((Math.random() * 255) + 0.5) });
		epc.put("PC", new Integer((int) (Math.random() * Short.MAX_VALUE)));
		epc.put("XPC_W1", new Integer(0));
		epc.put("XPC_W2", new Integer(0));
		scanData.put("Epc", epc);

		// scanData.put("String", "0f00");

		data.put("ScanData", scanData);

		data.put("CodeType", "EPC");

		data.put("Timestamp", new Long(new Date().getTime()));

		res.put("NS4|Numeric|6042", data);

		res.put("NS4|Numeric|6049", "Hello");

		return res;
	}

	public static void main(String[] args) {

		final OPCUA opcua = new OPCUA();
		final OPCUADataProvider serverSide = new OPCUADataProvider();

		try {

			if (args.length > 0 && args[0].equals("client")) {

				final Counter counter = new Counter();
				HashMap<String, String> properties = new HashMap<String, String>();
				properties.put("host", "10.65.54.174:4840");
				properties.put("publishInterval", "10");
				opcua.open(properties, new MessageHandler() {

					/**
					*
					*/
					private static final long serialVersionUID = 1L;

					@Override
					public void valueChanged(Object id, Object params) {
						System.out.println("Value " + id);
						System.out.println(" Param " + params);
						System.out.println("Time " + new Date());

						if (counter.counter == 10000) {
							opcua.close();
							System.exit(1);
						}
						counter.counter++;
					}

					@Override
					public void usabilityChanged(Object var, boolean isUsable) {
						// System.out.println("Usable " + var + " " + isUsable);

					}

					@Override
					public void messageReceived(Object var) {
						System.out.println("Published " + var);
					}
				});
				Object var = null;
				Object[] arr = new Object[5];
				
				arr[0] = new Integer(2);
				arr[1] = new Integer(2);
				arr[2] = new Integer(2);
				arr[3] = new Integer(2);
				ArrayList<Object> data = new ArrayList<Object>();
				data.add(new Integer(-54));
				data.add(new Integer(-2));
				arr[4] = data;

				//Object iArr[] = arr[4];
				
				
				
				arr = (Object[]) opcua.exec(3, "itsd", 3, "itsd.WriteProgram", arr);
				opcua.close();
			} else {
				serverSide.open(null, new DataProvider() {

					/**
					*
					*/
					private static final long serialVersionUID = 1L;
					Object value = "Hello";

					@Override
					public void write(int namespace, Object id, Object value) throws Exception {
						System.out.println("Write " + id + "@" + value);

						if (value.equals("Hugo")) {
							throw new OPCUAException("Error 40");
						} else {
							this.value = value;
						}

					}

					@Override
					public void unsubscribe(int namespace, Object id) throws Exception {

					}

					@Override
					public void subscribe(int namespace, Object id) throws Exception {

					}

					@Override
					public Object read(int namespace, Object id) throws Exception {
						if (id instanceof String) {
							String sId = (String) id;
							if (sId.endsWith("Program")) {
								System.out.println("Reading file");
								File file = new File("test.tar.gz");
								byte[] bFile = new byte[(int) file.length()];
								try (FileInputStream fInput = new FileInputStream(file)) {
									fInput.read(bFile);
								}
								Byte[] program = new Byte[bFile.length];
								int cnt = 0;
								for (byte b : bFile) {
									program[cnt] = b;
									cnt++;
								}

								return program;
							}
						}
						return value;
					}

					@SuppressWarnings("unchecked")
					@Override
					public Object exec(int namespaceMethod, Object methodId, int nodeNamespace, Object nodeId, Object params) throws Exception {
						System.out.println("Exec with " + params);
						if (params instanceof Object[]) {
							for (Object o : (Object[]) params) {
								if (o instanceof ArrayList) {
									ArrayList<Object> o2 = (ArrayList<Object>) o;
									byte[] data = new byte[o2.size()];
									int cnt = 0;
									for (Object oi : o2) {
										data[cnt] = (byte) oi;
										cnt++;
									}

									try (FileOutputStream fileOutput = new FileOutputStream("test.tar.gz")) {
										fileOutput.write(data);
										serverSide.notification(3, "itsd.Program", o2.toArray());
									}
								}
							}
						}
						return new Object[0];
					}
				});

				byte[] array = new byte[7]; // length is bounded by 7
				new Random().nextBytes(array);
				String generatedString = new String(array, Charset.forName("US-ASCII"));

				// serverSide.notification(4, "rfr310.DeviceName",
				// generatedString);
				//
				// new Random().nextBytes(array);
				// generatedString = new String(array,
				// Charset.forName("US-ASCII"));
				//
				// serverSide.notification(4, "rfr310.DeviceManual",
				// generatedString);
				//
				// new Random().nextBytes(array);
				// generatedString = new String(array,
				// Charset.forName("US-ASCII"));
				//
				// serverSide.notification(4, "rfr310.DeviceInfo",
				// generatedString);
				//
				// for (int i=0; i<30; i++) {
				// System.out.println("Event " + randomResult().toString());
				// serverSide.event(4, new Integer(1006), 4, "rfr310", new
				// Date().getTime(), 500, "Hello " + i, randomResult());
				// Thread.sleep(300);
			}
			System.in.read();
		} catch (Throwable t) {
			t.printStackTrace();
			System.exit(1);
		}

	}
}
