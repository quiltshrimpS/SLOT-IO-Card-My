using CommandMessenger;
using CommandMessenger.Transport.Serial;

namespace Spark.Slot.IO
{
	public class IOCard
	{
		public enum Commands
		{
			CMD_ACK = 0x00,
			CMD_GET_INFO = 0x01,
			CMD_GET_KEY_MASKS = 0x02,
			CMD_GET_KEYS = 0x10,
			CMD_SET_OUTPUT = 0x11,
			CMD_GET_COIN_COUNTER = 0x20,
			CMD_RESET_COIN_COINTER = 0x21,
			CMD_TICK_AUDIT_COUNTER = 0x30,
			CMD_EJECT_COIN = 0x40,
			CMD_SET_TRACK_LEVEL = 0x41,
			CMD_SET_EJECT_TIMEOUT = 0x42,
			CMD_READ_STORAGE = 0x50,
			CMD_WRITE_STORAGE = 0x58
		}

		public enum Events
		{
			EVT_GET_INFO_RESULT = 0x01,
			EVT_KEY_MASKS_RESULT = 0x02,
			EVT_KEYS_RESULT = 0x10,
			EVT_COIN_COUNTER_RESULT = 0x20,
			EVT_READ_STORAGE_RESULT = 0x50,
			EVT_WRITE_STORAGE_RESULT = 0x58,
			EVT_DEBUG = 0xFE,
			EVT_ERROR = 0xFF
		}

		public enum Errors
		{
			ERR_UNKNOWN_ERROR = 0x00,
			ERR_EJECT_INTERRUPTED = 0x01,
			ERR_EJECT_TIMEOUT = 0x02,
			ERR_NOT_A_TRACK = 0x03,
			ERR_PROTECTED_STORAGE = 0x04,
			ERR_TOO_LONG = 0x05,
			ERR_NOT_A_COUNTER = 0x06,
			ERR_UNKNOWN_COMMAND = 0xFF
		}

		public enum ActiveLevel
		{
			ActiveLow = 0x00,
			ActiveHigh = 0x01
		}

		/// <summary>
		/// queues a GET_INFO command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
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
		/// queues a EJECT_COIN command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="track"><c>CoinTrack</c> indicates which track to eject</param>
		/// <param name="count">number of coins to be ejected.</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryEjectCoin(byte track, byte count, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_EJECT_COIN);
				cmd.AddBinArgument(track);
				cmd.AddBinArgument(count);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a GET_COIN_COUNTER command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="track"><c>CoinTrack</c> indicates which track to get</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryGetCoinCounter(byte track, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_GET_COIN_COUNTER);
				cmd.AddBinArgument(track);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a GET_KEYS command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
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
		/// queues a GET_KEY_MASKS command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryGetKeyMasks(SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				mMessenger.SendCommand(new SendCommand((int)Commands.CMD_GET_KEY_MASKS), queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a SET_EJECT_TIMEOUT command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="track">Track.</param>
		/// <param name="timeout">
		/// Timeout, in microseconds. Timeouts less than 1000us might not work, since processing takes time...
		/// </param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QuerySetEjectTimeout(byte track, uint timeout, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_SET_EJECT_TIMEOUT);
				cmd.AddBinArgument(track);
				cmd.AddBinArgument(timeout);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a SET_OUTPUT command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="outputs">Outputs.</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.AtEndQueue</c>.
		/// </param>
		public bool QuerySetOutput(byte[] outputs, SendQueue queuePosition = SendQueue.AtEndQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_SET_OUTPUT);
				cmd.AddBinArgument((byte)outputs.Length);
				foreach (var b in outputs)
					cmd.AddBinArgument(b);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a SET_TRACK_LEVEL command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="track">Track.</param>
		/// <param name="level">Level.</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QuerySetTrackLevel(byte track, ActiveLevel level, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_SET_TRACK_LEVEL);
				cmd.AddBinArgument(track);
				cmd.AddBinArgument((byte)level);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a TICK_AUDIT_COUNTER command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="counter">Counter.</param>
		/// <param name="ticks">Ticks.</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryTickAuditCounter(byte counter, uint ticks, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_TICK_AUDIT_COUNTER);
				cmd.AddBinArgument(counter);
				cmd.AddBinArgument(ticks);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a WRITE_STORAGE command
		/// </summary>
		/// <returns><c>true</c>, if the command was queued, <c>false</c> otherwise.</returns>
		/// <param name="address">Address.</param>
		/// <param name="data">Data.</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryWriteStorage(ushort address, byte[] data, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_WRITE_STORAGE);
				cmd.AddBinArgument(address);
				cmd.AddBinArgument((byte)data.Length);
				for (int i = 0; i < data.Length; ++i)
					cmd.AddBinArgument(data[i]);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a READ_STORAGE command
		/// </summary>
		/// <returns><c>true</c>, if read storage was queryed, <c>false</c> otherwise.</returns>
		/// <param name="address">Address.</param>
		/// <param name="length">Length.</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.AtEndQueue</c>.
		/// </param>
		public bool QueryReadStorage(ushort address, byte length, SendQueue queuePosition = SendQueue.AtEndQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_READ_STORAGE);
				cmd.AddBinArgument(address);
				cmd.AddBinArgument(length);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// queues a RESET_COIN_COUNTER command
		/// </summary>
		/// <returns><c>true</c>, if reset coin counter was queryed, <c>false</c> otherwise.</returns>
		/// <param name="track"><c>CoinTrack</c> indicates which track to reset</param>
		/// <param name="queuePosition">
		/// position of the command to be placed, either <c>SendQueue.InFrontQueue</c> to place the command in front of
		/// the queue, or <c>SendQueue.AtEndQueue</c> to place the command at the end of the queue. Defaults to
		/// <c>SendQueue.InFrontQueue</c>.
		/// </param>
		public bool QueryResetCoinCounter(byte track, SendQueue queuePosition = SendQueue.InFrontQueue)
		{
			if (IsConnected)
			{
				var cmd = new SendCommand((int)Commands.CMD_RESET_COIN_COINTER);
				cmd.AddBinArgument(track);
				mMessenger.SendCommand(cmd, queuePosition);
				return true;
			}
			return false;
		}

		/// <summary>
		/// Connect to the IOCard with the specified port and baudrate.
		/// </summary>
		/// <param name="port">Port, ex. "COM10" on Windows, or "/dev/ttyUSB0" on Linux</param>
		/// <param name="baudrate">Baudrate.</param>
		/// <exception cref="System.InvalidOperationException">Thrown on connection fails (already connected, port busy, etc.)</exception>
		public void Connect(string port, int baudrate)
		{
			lock (this)
			{
				if (mMessenger != null)
					throw new System.InvalidOperationException("Already connected.");

				var messenger = new CmdMessenger(
					new SerialTransport { CurrentSerialSettings = { PortName = port, BaudRate = baudrate, DtrEnable = false } },
					512
				);
				if (messenger.Connect())
				{
					mMessenger = messenger;
					_attachCallbacks();
					if (OnConnected != null)
						OnConnected(this, System.EventArgs.Empty);
					return;
				}

				throw new System.InvalidOperationException("Connecting failed for unknown reason.");
			}
		}

		/// <summary>
		/// Disconnect from the IOCard
		/// </summary>
		public bool Disconnect()
		{
			lock (this)
			{
				if (IsConnected)
				{
					var status = mMessenger.Disconnect();
					if (status)
					{
						mMessenger = null;
						if (OnDisconnected != null)
							OnDisconnected(this, System.EventArgs.Empty);
					}
					return status;
				}
				return false;
			}
		}

		void _attachCallbacks()
		{
			mMessenger.Attach((int)Events.EVT_GET_INFO_RESULT, (receivedCommand) =>
			{
				string manufacturer = receivedCommand.ReadBinStringArg();
				string product = receivedCommand.ReadBinStringArg();
				string version = receivedCommand.ReadBinStringArg();
				uint protocol = receivedCommand.ReadBinUInt32Arg();

				if (OnGetInfoResult != null)
					OnGetInfoResult(this, new GetInfoResultEventArgs(receivedCommand.TimeStamp, manufacturer, product, version, protocol));
			});
			mMessenger.Attach((int)Events.EVT_COIN_COUNTER_RESULT, (receivedCommand) =>
			{
				// ACK this event so ejection don't get interruptted.
				if (IsConnected)
					mMessenger.SendCommand(new SendCommand((int)Commands.CMD_ACK), SendQueue.InFrontQueue);

				byte track = receivedCommand.ReadBinByteArg();
				uint coins = receivedCommand.ReadBinUInt32Arg();

				if (OnCoinCounterResult != null)
					OnCoinCounterResult(this, new CoinCounterResultEventArgs(receivedCommand.TimeStamp, track, coins));
			});
			mMessenger.Attach((int)Events.EVT_KEY_MASKS_RESULT, (receivedCommand) =>
			{
				var count = receivedCommand.ReadBinByteArg();
				var masks = new byte[count];
				for (int i = 0; i < count; ++i)
					masks[i] = receivedCommand.ReadBinByteArg();

				if (OnKeyMasks != null)
					OnKeyMasks(this, new KeyMasksEventArgs(receivedCommand.TimeStamp, masks));
			});
			mMessenger.Attach((int)Events.EVT_KEYS_RESULT, (receivedCommand) =>
			{
				var count = receivedCommand.ReadBinByteArg();
				var keys = new byte[count];
				for (int i = 0; i < count; ++i)
					keys[i] = receivedCommand.ReadBinByteArg();

				if (OnKeys != null)
					OnKeys(this, new KeysEventArgs(receivedCommand.TimeStamp, keys));
			});
			mMessenger.Attach((int)Events.EVT_WRITE_STORAGE_RESULT, (receivedCommand) =>
			{
				var address = receivedCommand.ReadBinUInt16Arg();
				var length = receivedCommand.ReadBinByteArg();

				if (OnWriteStorageResult != null)
					OnWriteStorageResult(this, new WriteStorageResultEventArgs(receivedCommand.TimeStamp, address, length));
			});
			mMessenger.Attach((int)Events.EVT_READ_STORAGE_RESULT, (receivedCommand) =>
			{
				var address = receivedCommand.ReadBinUInt16Arg();
				var length = receivedCommand.ReadBinByteArg();
				var data = new byte[length];
				for (int i = 0; i < length; ++i)
					data[i] = receivedCommand.ReadBinByteArg();

				if (OnReadStorageResult != null)
					OnReadStorageResult(this, new ReadStorageResultEventArgs(receivedCommand.TimeStamp, address, data));
			});
			mMessenger.Attach((int)Events.EVT_ERROR, (receivedCommand) =>
			{
				ErrorEventArgs e = null;
				var err = (Errors)receivedCommand.ReadBinByteArg();
				switch (err)
				{
					case Errors.ERR_EJECT_INTERRUPTED:
						{
							var track = receivedCommand.ReadBinByteArg();
							var coins = receivedCommand.ReadBinByteArg();
							e = new ErrorEjectInterruptedEventArgs(receivedCommand.TimeStamp, err, track, coins);
						}
						break;
					case Errors.ERR_EJECT_TIMEOUT:
						{
							var track = receivedCommand.ReadBinByteArg();
							var coins = receivedCommand.ReadBinByteArg();
							e = new ErrorEjectTimeoutEventArgs(receivedCommand.TimeStamp, err, track, coins);
						}
						break;
					case Errors.ERR_NOT_A_TRACK:
						e = new ErrorNotATrackEventArgs(receivedCommand.TimeStamp, err, receivedCommand.ReadBinByteArg());
						break;
					case Errors.ERR_NOT_A_COUNTER:
						e = new ErrorNotACounterEventArgs(receivedCommand.TimeStamp, err, receivedCommand.ReadBinByteArg());
						break;
					case Errors.ERR_PROTECTED_STORAGE:
						e = new ErrorProtectedStorageEventArgs(receivedCommand.TimeStamp, err, receivedCommand.ReadBinUInt16Arg());
						break;
					case Errors.ERR_TOO_LONG:
						{
							var desired = receivedCommand.ReadBinByteArg();
							var requested = receivedCommand.ReadBinByteArg();
							e = new ErrorTooLongEventArgs(receivedCommand.TimeStamp, err, desired, requested);
						}
						break;
					case Errors.ERR_UNKNOWN_COMMAND:
						e = new ErrorUnknownCommandEventArgs(receivedCommand.TimeStamp, err, receivedCommand.ReadBinByteArg());
						break;
					default:
						e = new ErrorUnknownErrorEventArgs(receivedCommand.TimeStamp, err, receivedCommand.ReadBinByteArg());
						break;
				}

				if (OnError != null)
					OnError(this, e);
			});
			mMessenger.Attach((int)Events.EVT_DEBUG, (receivedCommand) =>
			{
				if (OnDebug != null)
					OnDebug(this, new DebugEventArgs(receivedCommand.TimeStamp, receivedCommand.ReadBinStringArg()));
			});
			mMessenger.Attach((receivedCommand) =>
			{
				if (OnUnknown != null)
					OnUnknown(this, new UnknownEventArgs(receivedCommand.TimeStamp, receivedCommand));
			});
		}

		public bool IsConnected { get { lock (this) { return mMessenger != null; } } }

		public event System.EventHandler OnConnected;
		public event System.EventHandler OnDisconnected;
		public event System.EventHandler<GetInfoResultEventArgs> OnGetInfoResult;
		public event System.EventHandler<CoinCounterResultEventArgs> OnCoinCounterResult;
		public event System.EventHandler<KeysEventArgs> OnKeys;
		public event System.EventHandler<KeyMasksEventArgs> OnKeyMasks;
		public event System.EventHandler<WriteStorageResultEventArgs> OnWriteStorageResult;
		public event System.EventHandler<ReadStorageResultEventArgs> OnReadStorageResult;
		public event System.EventHandler<ErrorEventArgs> OnError;
		public event System.EventHandler<UnknownEventArgs> OnUnknown;
		public event System.EventHandler<DebugEventArgs> OnDebug;

		CmdMessenger mMessenger;

		#region "EventArgs"

		public class EventArgs : System.EventArgs
		{
			System.DateTime? mDateTime;

			public long TimeStamp { get; internal set; }
			public System.DateTime DateTime
			{
				get
				{
					if (!mDateTime.HasValue)
						mDateTime = new System
							.DateTime(1970, 1, 1, 0, 0, 0, 0, System.DateTimeKind.Utc)
							.AddMilliseconds(TimeStamp)
							.ToLocalTime();
					return mDateTime.Value;
				}
			}

			public EventArgs(long timestamp)
			{
				TimeStamp = timestamp;
			}
		}

		public class GetInfoResultEventArgs : EventArgs
		{
			public string Manufacturer { get; private set; }
			public string Product { get; private set; }
			public string Version { get; private set; }
			public uint ProtocolVersion { get; private set; }

			public GetInfoResultEventArgs(long timestamp, string manufacturer, string product, string version, uint protoVersion) :
				base(timestamp)
			{
				Manufacturer = manufacturer;
				Product = product;
				Version = version;
				ProtocolVersion = protoVersion;
			}
		}

		public class CoinCounterResultEventArgs : EventArgs
		{
			public byte Track { get; internal set; }
			public uint Coins { get; internal set; }

			public CoinCounterResultEventArgs(long timestamp, byte track, uint coins) :
				base(timestamp)
			{
				Track = track;
				Coins = coins;
			}
		}

		public class KeyMasksEventArgs : EventArgs
		{
			public byte[] KeyMasks { get; internal set; }

			public KeyMasksEventArgs(long timestamp, byte[] masks) :
				base(timestamp)
			{
				KeyMasks = masks;
			}
		}

		public class KeysEventArgs : EventArgs
		{
			public byte[] Keys { get; internal set; }

			public KeysEventArgs(long timestamp, byte[] keys) :
				base(timestamp)
			{
				Keys = keys;
			}
		}

		public class WriteStorageResultEventArgs : EventArgs
		{
			public ushort Address { get; internal set; }
			public byte Length { get; internal set; }

			public WriteStorageResultEventArgs(long timestamp, ushort address, byte length) :
				base(timestamp)
			{
				Address = address;
				Length = length;
			}
		}

		public class ReadStorageResultEventArgs : EventArgs
		{
			public ushort Address { get; internal set; }
			public byte[] Data { get; internal set; }

			public ReadStorageResultEventArgs(long timestamp, ushort address, byte[] data) :
				base(timestamp)
			{
				Address = address;
				Data = data;
			}
		}

		public class ErrorEventArgs : EventArgs
		{
			public Errors ErrorCode { get; internal set; }

			public ErrorEventArgs(long timestamp, Errors error) :
				base(timestamp)
			{
				ErrorCode = error;
			}
		}

		public class ErrorTrackEventArgs : ErrorEventArgs
		{
			public byte Track { get; internal set; }

			public ErrorTrackEventArgs(long timestamp, Errors error, byte track) :
				base(timestamp, error)
			{
				Track = track;
			}
		}

		public class ErrorEjectInterruptedEventArgs : ErrorTrackEventArgs
		{
			public byte CoinsFailed { get; internal set; }

			public ErrorEjectInterruptedEventArgs(long timestamp, Errors error, byte track, byte coins) :
				base(timestamp, error, track)
			{
				CoinsFailed = coins;
			}
		}

		public class ErrorEjectTimeoutEventArgs : ErrorTrackEventArgs
		{
			public byte CoinsFailed { get; internal set; }

			public ErrorEjectTimeoutEventArgs(long timestamp, Errors error, byte track, byte coins) :
				base(timestamp, error, track)
			{
				CoinsFailed = coins;
			}
		}

		public class ErrorNotATrackEventArgs : ErrorTrackEventArgs
		{
			public ErrorNotATrackEventArgs(long timestamp, Errors error, byte track) :
				base(timestamp, error, track)
			{
			}
		}

		public class ErrorNotACounterEventArgs : ErrorEventArgs
		{
			public byte AuditCounter { get; internal set; }

			public ErrorNotACounterEventArgs(long timestamp, Errors error, byte counter) :
				base(timestamp, error)
			{
				AuditCounter = counter;
			}
		}

		public class ErrorProtectedStorageEventArgs : ErrorEventArgs
		{
			public ushort Address { get; internal set; }

			public ErrorProtectedStorageEventArgs(long timestamp, Errors error, ushort address) :
				base(timestamp, error)
			{
				Address = address;
			}
		}

		public class ErrorTooLongEventArgs : ErrorEventArgs
		{
			public byte DesiredLength { get; internal set; }
			public byte RequestedLength { get; internal set; }

			public ErrorTooLongEventArgs(long timestamp, Errors error, byte desired, byte requested) :
				base(timestamp, error)
			{
				DesiredLength = desired;
				RequestedLength = requested;
			}
		}

		public class ErrorUnknownCommandEventArgs : ErrorEventArgs
		{
			public ushort Command { get; internal set; }

			public ErrorUnknownCommandEventArgs(long timestamp, Errors error, ushort command) :
				base(timestamp, error)
			{
				Command = command;
			}
		}

		public class ErrorUnknownErrorEventArgs : ErrorEventArgs
		{
			public ushort UnknownErrorCode { get; internal set; }

			public ErrorUnknownErrorEventArgs(long timestamp, Errors error, ushort errorCode) :
				base(timestamp, error)
			{
				UnknownErrorCode = errorCode;
			}
		}

		public class DebugEventArgs : EventArgs
		{
			public string Message { get; internal set; }

			public DebugEventArgs(long timestamp, string message) :
				base(timestamp)
			{
				Message = message;
			}
		}

		public class UnknownEventArgs : EventArgs
		{
			public ReceivedCommand Command { get; internal set; }

			public UnknownEventArgs(long timestamp, ReceivedCommand command) :
				base(timestamp)
			{
				Command = command;
			}
		}

		#endregion
	}
}
