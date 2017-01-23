using System;
using System.Collections.Generic;

using CommandMessenger;
using CommandMessenger.Transport;
using CommandMessenger.Transport.Serial;

namespace Spark.Slot.IO
{
	public class IOCard
	{
		public enum CoinTrack
		{
			/* 0x00 ~ 0x7F are coin insert tracks */
			TrackInsert1 = 0x00, TrackInsert2 = 0x01, TrackInsert3 = 0x02, TrackInsert4 = 0x03,
			TrackInsert5 = 0x04, TrackInsert6 = 0x05, TrackInsert7 = 0x06, TrackInsert8 = 0x07,
			TrackInsert9 = 0x08, TrackInsert10 = 0x09, TrackInsert11 = 0x0A, TrackInsert12 = 0x0B,
			TrackInsert13 = 0x0C, TrackInsert14 = 0x0D, TrackInsert15 = 0x0E, TrackInsert16 = 0x0F,
			TrackInsert17 = 0x10, TrackInsert18 = 0x11, TrackInsert19 = 0x12, TrackInsert20 = 0x13,
			TrackInsert21 = 0x14, TrackInsert22 = 0x15, TrackInsert23 = 0x16, TrackInsert24 = 0x17,
			TrackInsert25 = 0x18, TrackInsert26 = 0x19, TrackInsert27 = 0x1A, TrackInsert28 = 0x1B,
			TrackInsert29 = 0x1C, TrackInsert30 = 0x1D, TrackInsert31 = 0x1E, TrackInsert32 = 0x1F,
			TrackInsert33 = 0x20, TrackInsert34 = 0x21, TrackInsert35 = 0x22, TrackInsert36 = 0x23,
			TrackInsert37 = 0x24, TrackInsert38 = 0x25, TrackInsert39 = 0x26, TrackInsert40 = 0x27,
			TrackInsert41 = 0x28, TrackInsert42 = 0x29, TrackInsert43 = 0x2A, TrackInsert44 = 0x2B,
			TrackInsert45 = 0x2C, TrackInsert46 = 0x2D, TrackInsert47 = 0x2E, TrackInsert48 = 0x2F,
			TrackInsert49 = 0x30, TrackInsert50 = 0x31, TrackInsert51 = 0x32, TrackInsert52 = 0x33,
			TrackInsert53 = 0x34, TrackInsert54 = 0x35, TrackInsert55 = 0x36, TrackInsert56 = 0x37,
			TrackInsert57 = 0x38, TrackInsert58 = 0x39, TrackInsert59 = 0x3A, TrackInsert60 = 0x3B,
			TrackInsert61 = 0x3C, TrackInsert62 = 0x3D, TrackInsert63 = 0x3E, TrackInsert64 = 0x3F,
			TrackInsert65 = 0x40, TrackInsert66 = 0x41, TrackInsert67 = 0x42, TrackInsert68 = 0x43,
			TrackInsert69 = 0x44, TrackInsert70 = 0x45, TrackInsert71 = 0x46, TrackInsert72 = 0x47,
			TrackInsert73 = 0x48, TrackInsert74 = 0x49, TrackInsert75 = 0x4A, TrackInsert76 = 0x4B,
			TrackInsert77 = 0x4C, TrackInsert78 = 0x4D, TrackInsert79 = 0x4E, TrackInsert80 = 0x4F,
			TrackInsert81 = 0x50, TrackInsert82 = 0x51, TrackInsert83 = 0x52, TrackInsert84 = 0x53,
			TrackInsert85 = 0x54, TrackInsert86 = 0x55, TrackInsert87 = 0x56, TrackInsert88 = 0x57,
			TrackInsert89 = 0x58, TrackInsert90 = 0x59, TrackInsert91 = 0x5A, TrackInsert92 = 0x5B,
			TrackInsert93 = 0x5C, TrackInsert94 = 0x5D, TrackInsert95 = 0x5E, TrackInsert96 = 0x5F,
			TrackInsert97 = 0x60, TrackInsert98 = 0x61, TrackInsert99 = 0x62, TrackInsert100 = 0x63,
			TrackInsert101 = 0x64, TrackInsert102 = 0x65, TrackInsert103 = 0x66, TrackInsert104 = 0x67,
			TrackInsert105 = 0x68, TrackInsert106 = 0x69, TrackInsert107 = 0x6A, TrackInsert108 = 0x6B,
			TrackInsert109 = 0x6C, TrackInsert110 = 0x6D, TrackInsert111 = 0x6E, TrackInsert112 = 0x6F,
			TrackInsert113 = 0x70, TrackInsert114 = 0x71, TrackInsert115 = 0x72, TrackInsert116 = 0x73,
			TrackInsert117 = 0x74, TrackInsert118 = 0x75, TrackInsert119 = 0x76, TrackInsert120 = 0x77,
			TrackInsert121 = 0x78, TrackInsert122 = 0x79, TrackInsert123 = 0x7A, TrackInsert124 = 0x7B,
			TrackInsert125 = 0x7C, TrackInsert126 = 0x7D, TrackInsert127 = 0x7E, TrackInsert128 = 0x7F,

			/* 0x80 ~ 0xBF are banknote insert tracks */
			TrackBanknote1 = 0x80, TrackBanknote2 = 0x81, TrackBanknote3 = 0x82, TrackBanknote4 = 0x83,
			TrackBanknote5 = 0x84, TrackBanknote6 = 0x85, TrackBanknote7 = 0x86, TrackBanknote8 = 0x87,
			TrackBanknote9 = 0x88, TrackBanknote10 = 0x89, TrackBanknote11 = 0x8A, TrackBanknote12 = 0x8B,
			TrackBanknote13 = 0x8C, TrackBanknote14 = 0x8D, TrackBanknote15 = 0x8E, TrackBanknote16 = 0x8F,
			TrackBanknote17 = 0x90, TrackBanknote18 = 0x91, TrackBanknote19 = 0x92, TrackBanknote20 = 0x93,
			TrackBanknote21 = 0x94, TrackBanknote22 = 0x95, TrackBanknote23 = 0x96, TrackBanknote24 = 0x97,
			TrackBanknote25 = 0x98, TrackBanknote26 = 0x99, TrackBanknote27 = 0x9A, TrackBanknote28 = 0x9B,
			TrackBanknote29 = 0x9C, TrackBanknote30 = 0x9D, TrackBanknote31 = 0x9E, TrackBanknote32 = 0x9F,
			TrackBanknote33 = 0xA0, TrackBanknote34 = 0xA1, TrackBanknote35 = 0xA2, TrackBanknote36 = 0xA3,
			TrackBanknote37 = 0xA4, TrackBanknote38 = 0xA5, TrackBanknote39 = 0xA6, TrackBanknote40 = 0xA7,
			TrackBanknote41 = 0xA8, TrackBanknote42 = 0xA9, TrackBanknote43 = 0xAA, TrackBanknote44 = 0xAB,
			TrackBanknote45 = 0xAC, TrackBanknote46 = 0xAD, TrackBanknote47 = 0xAE, TrackBanknote48 = 0xAF,
			TrackBanknote49 = 0xB0, TrackBanknote50 = 0xB1, TrackBanknote51 = 0xB2, TrackBanknote52 = 0xB3,
			TrackBanknote53 = 0xB4, TrackBanknote54 = 0xB5, TrackBanknote55 = 0xB6, TrackBanknote56 = 0xB7,
			TrackBanknote57 = 0xB8, TrackBanknote58 = 0xB9, TrackBanknote59 = 0xBA, TrackBanknote60 = 0xBB,
			TrackBanknote61 = 0xBC, TrackBanknote62 = 0xBD, TrackBanknote63 = 0xBE, TrackBanknote64 = 0xBF,

			/* 0xC0 ~ 0xFF are coin eject tracks */
			TrackEject1 = 0xC0, TrackEject2 = 0xC1, TrackEject3 = 0xC2, TrackEject4 = 0xC3,
			TrackEject5 = 0xC4, TrackEject6 = 0xC5, TrackEject7 = 0xC6, TrackEject8 = 0xC7,
			TrackEject9 = 0xC8, TrackEject10 = 0xC9, TrackEject11 = 0xCA, TrackEject12 = 0xCB,
			TrackEject13 = 0xCC, TrackEject14 = 0xCD, TrackEject15 = 0xCE, TrackEject16 = 0xCF,
			TrackEject17 = 0xD0, TrackEject18 = 0xD1, TrackEject19 = 0xD2, TrackEject20 = 0xD3,
			TrackEject21 = 0xD4, TrackEject22 = 0xD5, TrackEject23 = 0xD6, TrackEject24 = 0xD7,
			TrackEject25 = 0xD8, TrackEject26 = 0xD9, TrackEject27 = 0xDA, TrackEject28 = 0xDB,
			TrackEject29 = 0xDC, TrackEject30 = 0xDD, TrackEject31 = 0xDE, TrackEject32 = 0xDF,
			TrackEject33 = 0xE0, TrackEject34 = 0xE1, TrackEject35 = 0xE2, TrackEject36 = 0xE3,
			TrackEject37 = 0xE4, TrackEject38 = 0xE5, TrackEject39 = 0xE6, TrackEject40 = 0xE7,
			TrackEject41 = 0xE8, TrackEject42 = 0xE9, TrackEject43 = 0xEA, TrackEject44 = 0xEB,
			TrackEject45 = 0xEC, TrackEject46 = 0xED, TrackEject47 = 0xEE, TrackEject48 = 0xEF,
			TrackEject49 = 0xF0, TrackEject50 = 0xF1, TrackEject51 = 0xF2, TrackEject52 = 0xF3,
			TrackEject53 = 0xF4, TrackEject54 = 0xF5, TrackEject55 = 0xF6, TrackEject56 = 0xF7,
			TrackEject57 = 0xF8, TrackEject58 = 0xF9, TrackEject59 = 0xFA, TrackEject60 = 0xFB,
			TrackEject61 = 0xFC, TrackEject62 = 0xFD, TrackEject63 = 0xFE, TrackEject64 = 0xFF,
		}

		public enum ButtonState
		{
			StatePressed,
			StateReleased,
			/// <summary>
			///  button state not (yet) available.
			/// </summary>
			/// <value>
			/// 1. not received any update, yet
			/// 2. it's "masked", not an available input.
			/// </value>
			StateUnavailable,
		}

		public IOCard()
		{
		}

		/// <summary>
		/// queue a GET_INFO command
		/// </summary>
		/// <returns><c>true</c>, if GET_INFO was queued, <c>false</c> otherwise.</returns>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.AtEndQueue</c>.
		/// </param>
		public bool QueryGetInfo(SendQueue queuePosition = SendQueue.AtEndQueue)
		{
			if (IsConnected)
			{
				mMessenger.SendCommand(new SendCommand((int)Commands.CMD_GET_INFO), queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queue a EJECT_COIN command
		/// </summary>
		/// <returns><c>true</c>, if EJECT_COIN was queued, <c>false</c> otherwise.</returns>
		/// <param name="track"><c>CoinTrack</c> indicates which track to eject</param>
		/// <param name="count">number of coins to be ejected.</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryEjectCoin(CoinTrack track, byte count, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_EJECT_COIN);
				cmd.AddBinArgument((uint)track);
				cmd.AddBinArgument((uint)count);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queue a GET_COIN_COUNTER command
		/// </summary>
		/// <returns><c>true</c>, if GET_COIN_COUNTER was queued, <c>false</c> otherwise.</returns>
		/// <param name="track"><c>CoinTrack</c> indicates which track to get</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryGetCoinCounter(CoinTrack track, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_GET_COIN_COUNTER);
				cmd.AddBinArgument((uint)track);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queue a GET_KEYS command
		/// </summary>
		/// <returns><c>true</c>, if GET_KEYS was queued, <c>false</c> otherwise.</returns>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.AtEndQueue</c>.
		/// </param>
		public bool QueryGetKeys(SendQueue queuePosition = SendQueue.AtEndQueue)
		{
			if (IsConnected)
			{
				mMessenger.SendCommand(new SendCommand((int)Commands.CMD_GET_KEYS), queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queue a RESET_COIN_COUNTER command
		/// </summary>
		/// <returns><c>true</c>, if reset coin counter was queryed, <c>false</c> otherwise.</returns>
		/// <param name="track"><c>CoinTrack</c> indicates which track to reset</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryResetCoinCounter(CoinTrack track, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_RESET_COIN_COINTER);
				cmd.AddBinArgument((uint)track);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// Connect the specified port and baudrate.
		/// </summary>
		/// <param name="port">Port.</param>
		/// <param name="baudrate">Baudrate.</param>
		public bool Connect(string port, int baudrate)
		{
			if (mMessenger != null)
				return false;

			try
			{
				var transport = new SerialTransport()
				{
					CurrentSerialSettings = { PortName = port, BaudRate = baudrate, DtrEnable = false, },
				};

				var messenger = new CmdMessenger(transport);
				messenger.Attach((int)Events.EVT_GET_INFO_RESULT, (receivedCommand) =>
				{
					string manufacturer = receivedCommand.ReadBinStringArg();
					string product = receivedCommand.ReadBinStringArg();
					string version = receivedCommand.ReadBinStringArg();
					UInt32 protocol = receivedCommand.ReadBinUInt32Arg();

					if (OnGetInfoResult != null)
						OnGetInfoResult(this, new GetInfoResultEventArgs(manufacturer, product, version, protocol));
				});
				messenger.Attach((int)Events.EVT_COIN_COUNTER_RESULT, (receivedCommand) =>
				{
					int track = receivedCommand.ReadBinByteArg();
					UInt32 coins = receivedCommand.ReadBinUInt32Arg();

					if (OnCoinCounterResult != null)
						OnCoinCounterResult(this, new CoinCounterResultEventArgs(track, coins));
				});
				messenger.Attach((int)Events.EVT_KEY, (receivedCommand) =>
				{
					var count = receivedCommand.ReadBinByteArg();
					var keys = new byte[count];
					for (int i = 0; i < count; ++i)
						keys[i] = receivedCommand.ReadBinByteArg();

					if (OnKey != null)
						OnKey(this, new KeyEventArgs(keys));
				});
				messenger.Attach((int)Events.EVT_ERROR, (receivedCommand) =>
				{
					ErrorEventArgs e = null;
					var err = (Errors)receivedCommand.ReadBinByteArg();
					switch (err)
					{
						case Errors.ERR_EJECT_INTERRUPTED:
							{
								var track = (CoinTrack)receivedCommand.ReadBinByteArg();
								var coins = receivedCommand.ReadBinByteArg();
								e = new ErrorEjectInterruptedEventArgs(err, track, coins);
							}
							break;
						case Errors.ERR_EJECT_TIMEOUT:
							e = new ErrorEjectTimeoutEventArgs(err, (CoinTrack)receivedCommand.ReadBinByteArg());
							break;
						case Errors.ERR_NOT_A_TRACK:
							e = new ErrorNotATrackEventArgs(err, (CoinTrack)receivedCommand.ReadBinByteArg());
							break;
						case Errors.ERR_UNKNOWN_COMMAND:
							e = new ErrorUnknownCommandEventArgs(err, receivedCommand.ReadBinByteArg());
							break;
						default:
							e = new ErrorUnknownErrorEventArgs(err, receivedCommand.ReadBinByteArg());
							break;
					}

					if (OnError != null)
						OnError(this, e);
				});
				messenger.Attach((receivedCommand) =>
				{
					if (OnUnknown != null)
						OnUnknown(this, new UnknownEventArgs(receivedCommand));
				});

				if (messenger.Connect())
				{
					mMessenger = messenger;
					if (OnConnected != null)
						OnConnected(this, EventArgs.Empty);
					return true;
				}
			}
			catch (InvalidOperationException ex)
			{
				Console.WriteLine("IOCard.Connect(): connection failed, " + ex.Message);
			}
			return false;
		}

		public bool Disconnect()
		{
			if (IsConnected)
			{
				var status = mMessenger.Disconnect();
				if (status)
				{
					mMessenger = null;
					if (OnDisconnected != null)
						OnDisconnected(this, EventArgs.Empty);
				}
				return status;
			}
			return false;
		}

		public bool IsConnected { get { return mMessenger != null; } }

		public event EventHandler OnConnected;
		public event EventHandler OnDisconnected;
		public event EventHandler<GetInfoResultEventArgs> OnGetInfoResult;
		public event EventHandler<CoinCounterResultEventArgs> OnCoinCounterResult;
		public event EventHandler<KeyEventArgs> OnKey;
		public event EventHandler<ErrorEventArgs> OnError;
		public event EventHandler<UnknownEventArgs> OnUnknown;

		#region "implementations"

		private CmdMessenger mMessenger = null;

		public enum Commands
		{
			CMD_READ_STORAGE = 0x87,
			CMD_WRITE_STORAGE = 0x96,
			CMD_SET_OUTPUT = 0xB4,
			CMD_RESET_COIN_COINTER = 0xC3,
			CMD_GET_COIN_COUNTER = 0xD2,
			CMD_EJECT_COIN = 0xE1,
			CMD_GET_INFO = 0xF0,
			CMD_GET_KEYS = 0xA5,
		}

		public enum Events
		{
			EVT_GET_INFO_RESULT = 0x0F,
			EVT_COIN_COUNTER_RESULT = 0x2D,
			EVT_KEY = 0x4B,
			EVT_WRITE_STORAGE_RESULT = 0x69,
			EVT_READ_STORAGE_RESULT = 0x78,
			EVT_ERROR = 0xFF,
		}

		public enum Errors
		{
			ERR_UNKNOWN_ERROR = 0x00,
			ERR_EJECT_INTERRUPTED = 0x01,
			ERR_EJECT_TIMEOUT = 0x02,
			ERR_NOT_A_TRACK = 0x03,
			ERR_UNKNOWN_COMMAND = 0xFF,
		}

		public class GetInfoResultEventArgs : EventArgs
		{
			public string Manufacturer { get; private set; }
			public string Product { get; private set; }
			public string Version { get; private set; }
			public UInt32 ProtocolVersion { get; private set; }

			public GetInfoResultEventArgs(string manufacturer, string product, string version, UInt32 protoVersion)
			{
				Manufacturer = manufacturer;
				Product = product;
				Version = version;
				ProtocolVersion = protoVersion;
			}
		}

		public class CoinCounterResultEventArgs : EventArgs
		{
			public int Track { get; internal set; }
			public UInt32 Coins { get; internal set; }

			public CoinCounterResultEventArgs(int track, UInt32 coins)
			{
				Track = track;
				Coins = coins;
			}
		}

		public class KeyEventArgs : EventArgs
		{
			public byte[] Keys { get; internal set; }

			public KeyEventArgs(byte[] keys)
			{
				Keys = keys;
			}
		}

		public class ErrorEventArgs : EventArgs
		{
			public Errors ErrorCode { get; internal set; }

			public ErrorEventArgs(Errors error)
			{
				ErrorCode = error;
			}
		}

		public class ErrorTrackEventArgs : ErrorEventArgs
		{
			public CoinTrack Track { get; internal set; }

			public ErrorTrackEventArgs(Errors error, CoinTrack track) :
				base(error)
			{
				Track = track;
			}
		}

		public class ErrorEjectInterruptedEventArgs : ErrorTrackEventArgs
		{
			public UInt16 CoinsFailed { get; internal set; }

			public ErrorEjectInterruptedEventArgs(Errors error, CoinTrack track, UInt16 coins) :
				base(error, track)
			{
				CoinsFailed = coins;
			}
		}

		public class ErrorEjectTimeoutEventArgs : ErrorTrackEventArgs
		{
			public ErrorEjectTimeoutEventArgs(Errors error, CoinTrack track) :
				base(error, track)
			{
			}
		}

		public class ErrorNotATrackEventArgs : ErrorTrackEventArgs
		{
			public ErrorNotATrackEventArgs(Errors error, CoinTrack track) :
				base(error, track)
			{
			}
		}

		public class ErrorUnknownCommandEventArgs : ErrorEventArgs
		{
			public UInt16 Command { get; internal set; }

			public ErrorUnknownCommandEventArgs(Errors error, UInt16 command) :
				base(error)
			{
				Command = command;
			}
		}

		public class ErrorUnknownErrorEventArgs : ErrorEventArgs
		{
			public UInt16 UnknownErrorCode { get; internal set; }

			public ErrorUnknownErrorEventArgs(Errors error, UInt16 errorCode) :
				base(error)
			{
				UnknownErrorCode = errorCode;
			}
		}

		public class UnknownEventArgs : EventArgs
		{
			public ReceivedCommand Command { get; internal set; }

			public UnknownEventArgs(ReceivedCommand command)
			{
				Command = command;
			}
		}

		#endregion
	}
}
