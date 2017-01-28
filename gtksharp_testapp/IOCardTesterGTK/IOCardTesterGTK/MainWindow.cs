using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Text;
using System.Text.RegularExpressions;
using Gtk;
using Spark.Slot.IO;

public partial class MainWindow : Window
{
	#region "Unity Integration Demo Codes"

	// the IOCard is purely event based, which is faster and more responsive.
	// However, to easy the development, create a <see cref="Spark.Slot.IO.IOCardStateCache"/> instance to cache the states.
	IOCardStateCache mCardCache = new IOCardStateCache(new IOCard());
	// or, you can instantiate the <see cref="Spark.Slot.IO.IOCardStateCache"/> with error capacity besides the default 3000.
	//IOCardStateCache mCardCache = new IOCardStateCache(new IOCard(), 1000);

	// this is how you connect to the card
	void Connect(string port, int baudrate)
	{
		try
		{
			// pass in the port and desired baudrate, for example "COM3" (on Windows) or "/dev/ttyUSB0" (on Linux)
			mCardCache.Card.Connect(port, baudrate);

			// after connected, the Cache does these 2 things for you
			//mCardCache.Card.QueryGetInfo();
			//mCardCache.Card.QueryGetKeyMasks();

			// you might also want to to query the initial key states here, else the key states are initially 
			// <see cref="Spark.Slot.IO.IOCardStateCache.KeyState.StateUnknown"/>.
			mCardCache.Card.QueryGetKeys();
		}
		catch (InvalidOperationException ex)
		{
			// catch the exception and handle it, here we just re-throw it.
			throw ex;
		}
	}

	// this is how you disconnect from the card
	void Disconnect()
	{
		try
		{
			// just call disconnect on the card to disconnect.
			mCardCache.Card.Disconnect();
		}
		catch (InvalidOperationException ex)
		{
			// catch the exception and handle it, here we just re-throw it.
			throw ex;
		}
	}

	// variables used in the demo
	DateTime mLedTime = DateTime.Now;
	static readonly byte[] outputs = { 0xff, 0xff, 0xff };

	// this function mimics MonoBehaviour.Update(), is called with an interval of (1000/60) milliseconds to simulate
	// Unity frame update.
	void Update()
	{
		// call <c>Spark.Slot.IO.IOCard.QuerySomething()</c> with coressponding arguments to queue a command to the card.
		// they return true if the command is queued, false otherwise.
		//
		// for example, here we toggle the onboard outputs (with SSR bits masked out)
		if (DateTime.Now - mLedTime > TimeSpan.FromSeconds(1.0))
		{
			mCardCache.Card.QuerySetOutput(outputs); // set outputs

			// toggle the bits :-D
			for (int i = 0; i < outputs.Length; ++i)
				outputs[i] = (byte)~outputs[i];

			mLedTime = DateTime.Now;
		}

		// there is an convinient <see cref="Spark.Slot.IO.IOCardStateCache.IsChanged"/>.
		// if it is false, means nothing has changed since the last <see cref="Spark.Slot.IO.IOCardStateCache.Processed()"/> call,
		// and you can safely skip every processing.
		if (mCardCache.IsChanged)
		{
			// get keys like this, depends on active level, normally active low.
			if (mCardCache.GetKey(0) == IOCardStateCache.KeyState.StateLow)
			{
				mCardCache.Card.QueryEjectCoin(0, 5); // eject 5 coins from track 0
			}

			// hit key 1 to tick the counters :-)
			if (mCardCache.GetKey(1) == IOCardStateCache.KeyState.StateLow)
			{
				// tick audit counter 0 for 100 times, and this command placed in front of the queue.
				mCardCache.Card.QueryTickAuditCounter(0, 100, CommandMessenger.SendQueue.InFrontQueue);
			}

			// get the errors like this
			while (!mCardCache.IsErrorQueueEmpty)
			{
				// pop an error
				var error = mCardCache.PopError();

				// null check, if other threads pops the error we might get null.
				if (error != null)
				{
					// cast the errors according to error codes to get more information from it
					switch (error.ErrorCode)
					{
						case IOCard.Errors.ERR_UNKNOWN_ERROR:
							{
								var e = (IOCard.ErrorUnknownErrorEventArgs)error;
								Debug.WriteLine("Got: error = {0}, time = {1}", e.ErrorCode, e.DateTime);
							}
							break;
						case IOCard.Errors.ERR_EJECT_INTERRUPTED:
							{
								var e = (IOCard.ErrorEjectInterruptedEventArgs)error;
								Debug.WriteLine("Got: error = {0}, track = {1}, coins failed = {2}, time = {3}", e.ErrorCode, e.Track, e.CoinsFailed, e.DateTime);
							}
							break;
						case IOCard.Errors.ERR_EJECT_TIMEOUT:
							{
								var e = (IOCard.ErrorEjectTimeoutEventArgs)error;
								Debug.WriteLine("Got: error = {0}, track = {1}, coins failed = {2}, time = {3}", e.ErrorCode, e.Track, e.CoinsFailed, e.DateTime);
							}
							break;
						case IOCard.Errors.ERR_NOT_A_TRACK:
							{
								var e = (IOCard.ErrorNotATrackEventArgs)error;
								Debug.WriteLine("Got: error = {0}, track = {1}, time = {2}", e.ErrorCode, e.Track, e.DateTime);
							}
							break;
						case IOCard.Errors.ERR_PROTECTED_STORAGE:
							{
								var e = (IOCard.ErrorProtectedStorageEventArgs)error;
								Debug.WriteLine("Got: error = {0}, address = 0x{1:X4}, time = {2}", e.ErrorCode, e.Address, e.DateTime);
							}
							break;
						case IOCard.Errors.ERR_TOO_LONG:
							{
								var e = (IOCard.ErrorTooLongEventArgs)error;
								Debug.WriteLine("Got: error = {0}, desired = {1}, requested = {2}, time = {3}", e.DesiredLength, e.RequestedLength, e.DateTime);
							}
							break;
						case IOCard.Errors.ERR_NOT_A_COUNTER:
							{
								var e = (IOCard.ErrorNotACounterEventArgs)error;
								Debug.WriteLine("Got: error = {0}, counter = {1}, time = {3}", e.ErrorCode, e.AuditCounter, e.DateTime);
							}
							break;
						case IOCard.Errors.ERR_UNKNOWN_COMMAND:
							{
								var e = (IOCard.ErrorUnknownCommandEventArgs)error;
								Debug.WriteLine("Got: error = {0}, command = {1}, time = {2}", e.ErrorCode, e.Command, e.DateTime);
							}
							break;
					}
				}
			}

			// call this to tell the cache that all changes are processed.
			mCardCache.Processed();
		}
	}

	#endregion

	#region "GTK# codes, safely ignores them if you're not interested."
	IOCard mCard { get { return mCardCache.Card; } }

	int mLastCmdIndex;

	List<string> mPorts = new List<string>(new string[] { "COM3", "COM1", "COM20" });

	class CommandProperty
	{
		public readonly IOCard.Commands Command;
		public readonly int Params;
		public readonly string CommandDescription;
		public readonly string ParamsDescription;
		public readonly List<string> HistoryParams;
		readonly Action<IOCard.Commands, Exception> OnErrorCallback;
		readonly Action<IOCard.Commands, string[]> SendCommandCallback;

		public CommandProperty(Action<IOCard.Commands, Exception> error_callback, IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc) :
			this(error_callback, cmd, parameter_count, cmdDesc, paramsDesc, new string[0], (command, parameters) => { })
		{
		}

		public CommandProperty(Action<IOCard.Commands, Exception> error_callback, IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc, string[] history) :
			this(error_callback, cmd, parameter_count, cmdDesc, paramsDesc, history, (command, parameters) => { })
		{
		}

		public CommandProperty(Action<IOCard.Commands, Exception> error_callback, IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc, Action<IOCard.Commands, string[]> callback) :
			this(error_callback, cmd, parameter_count, cmdDesc, paramsDesc, new string[0], callback)
		{
		}

		public CommandProperty(Action<IOCard.Commands, Exception> error_callback, IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc, string[] history, Action<IOCard.Commands, string[]> callback)
		{
			OnErrorCallback = error_callback;
			Command = cmd;
			Params = parameter_count;
			CommandDescription = cmdDesc;
			ParamsDescription = paramsDesc;
			HistoryParams = new List<string>(history);
			SendCommandCallback = callback;
		}

		public void SendCommand(string[] parameters)
		{
			try
			{
				SendCommandCallback.Invoke(Command, parameters);
			}
			catch (Exception ex)
			{
				if (OnErrorCallback != null)
					OnErrorCallback(Command, ex);
			}
		}
	}

	CommandProperty mCommandProperty_GetInfo;
	CommandProperty mCommandProperty_EjectCoin;
	CommandProperty mCommandProperty_GetCoinCounter;
	CommandProperty mCommandProperty_ResetCoinCounter;
	CommandProperty mCommandProperty_GetKeys;
	CommandProperty mCommandProperty_GetKeyMasks;
	CommandProperty mCommandProperty_SetEjectTimeout;
	CommandProperty mCommandProperty_TickAuditCounter;
	CommandProperty mCommandProperty_SetOutputs;
	CommandProperty mCommandProperty_SetTrackLevel;
	CommandProperty mCommandProperty_WriteStorage;
	CommandProperty mCommandProperty_ReadStorage;
	CommandProperty mCommandProperty_Reboot;
	CommandProperty[] mCommandProperties;

	public MainWindow() : base(WindowType.Toplevel)
	{
		Build();

		textview_received.ModifyFont(Pango.FontDescription.FromString("Monospace 8"));

		GLib.Timeout.Add(1000 / 60, delegate ()
		{
			if (checkbutton_update.Active)
				Update();
			return true;
		});

		Action<IOCard.Commands, Exception> on_error_callback = (command, ex) =>
		{
			var iter = textview_received.Buffer.StartIter;
			textview_received.Buffer.Insert(
				ref iter,
				string.Format(
					" => {0}: cmd {1} failed, reason {2}\r\n",
					DateTime.Now,
					command,
					ex.Message
				)
			);
		};

		mCommandProperty_GetInfo = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_GET_INFO, 0,
			"Get device information",
			"Params: N/A",
			(command, parameters) =>
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}\r\n",
						DateTime.Now,
						command
					)
				);
				mCard.QueryGetInfo();
			}
		);
		mCommandProperty_EjectCoin = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_EJECT_COIN, 2,
			"Eject N coins.",
			"Params: <track (byte)>, <coins (byte)>",
			new string[] {
				"0, 0x0A // eject 10 coins from track 0",
				"0, 5 // eject 5 coins from track 0",
				"0, 0 // interrupt the ongoing ejection on track 0"
			},
			(command, parameters) =>
			{
				var track = (byte)_getTfromString<uint>(parameters[0].Trim());
				var count = (byte)_getTfromString<uint>(parameters[1].Trim());

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, track = {2:X2}, count = {3}\r\n",
						DateTime.Now,
						command,
						track,
						count
					)
				);

				mCard.QueryEjectCoin(track, count);
			}
		);
		mCommandProperty_GetCoinCounter = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_GET_COIN_COUNTER, 1,
			"Get coin counter.",
			"Params: <track (byte)>",
			new string[] { "0 // track 0", "1 // track 1", "2 // track 2", "3 // track 2", "4 // track 4", "5 // track 5" },
			(command, parameters) =>
			{
				var track = (byte)_getTfromString<uint>(parameters[0].Trim());

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, track = 0x{2:X2}\r\n",
						DateTime.Now,
						command,
						track
					)
				);

				mCard.QueryGetCoinCounter(track);
			}
		);
		mCommandProperty_ResetCoinCounter = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_RESET_COIN_COINTER, 1,
			"Reset coin counter.",
			"Params: <track (byte)>",
			new string[] {
				"0x00 // track 0x00 (insert 1)",
				"0x80 // track 0x80 (banknote 1)",
				"0xC0 // track 0xC0 (eject track 1)"
			},
			(command, parameters) =>
			{
				var track = (byte)_getTfromString<uint>(parameters[0].Trim());

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, track = 0x{2:X2}\r\n",
						DateTime.Now,
						command,
						track
					)
				);

				mCard.QueryResetCoinCounter(track);
			}
		);
		mCommandProperty_SetEjectTimeout = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_SET_EJECT_TIMEOUT, 2,
			"Sets the Timeout on Eject tracks (in us)",
			"Params: <track (byte)>, <timeout_us (uint32_t)>",
			new string[] {
				"0, 5000000 // track 0xC0 (eject track 1) times out in 5 sec",
				"0xC0, 10000000 // track 0xC0 (eject track 1) times out in 10 sec"
			},
			(command, parameters) =>
			{
				var track = (byte)_getTfromString<uint>(parameters[0].Trim());
				var timeout = _getTfromString<uint>(parameters[1].Trim());

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, track = {2}, timeout = {3}us\r\n",
						DateTime.Now,
						command,
						track,
						timeout
					)
				);

				mCard.QuerySetEjectTimeout(track, timeout);
			}
		);
		mCommandProperty_SetTrackLevel = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_SET_TRACK_LEVEL, 2,
			"Sets the ActiveLevel on a track",
			"Params: <track (byte)>, <level (byte)>",
			new string[] {
				"0xC0, 0 // make track 0xC0 (eject track 1) active LOW",
				"0xC0, 1 // make track 0xC0 (eject track 1) active HIGH"
			},
			(command, parameters) =>
			{
				var track = (byte)_getTfromString<uint>(parameters[0].Trim());
				var level = (IOCard.ActiveLevel)_getTfromString<uint>(parameters[1].Trim());

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, track = {2}, level = {3}us\r\n",
						DateTime.Now,
						command,
						track,
						level
					)
				);

				mCard.QuerySetTrackLevel(track, level);
			}
		);
		mCommandProperty_GetKeys = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_GET_KEYS, 0,
			"Get key states from device",
			"Params: N/A",
			(command, parameters) =>
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}\r\n",
						DateTime.Now,
						command
					)
				);
				mCard.QueryGetKeys();
			}
		);
		mCommandProperty_GetKeyMasks = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_GET_KEY_MASKS, 0,
			"Get key masks from device",
			"Params: N/A",
			(command, parameters) =>
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}\r\n",
						DateTime.Now,
						command
					)
				);
				mCard.QueryGetKeyMasks();
			}
		);
		mCommandProperty_TickAuditCounter = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_TICK_AUDIT_COUNTER, 2,
			"Tick the audit counter",
			"Params: <counter (1 byte)>, <ticks (1 byte)>",
			new string[] {
				"0, 100 // tick the AuditCounter1 for 100 times",
				"1, 200 // tick the AuditCounter2 for 200 times",
				"2, 150 // tick the AuditCounter3 for 150 times",
				"3, 0 // tick the AuditCounter4 for 0 times (??)"
			},
			(command, parameters) =>
			{
				var counter = (byte)_getTfromString<uint>(parameters[0].Trim());
				var ticks = _getTfromString<uint>(parameters[1].Trim());

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, counter = 0x{2:X2}, ticks = {3}\r\n",
						DateTime.Now,
						command,
						counter,
						ticks
					)
				);

				mCard.QueryTickAuditCounter(counter, ticks);
			}
		);
		mCommandProperty_SetOutputs = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_SET_OUTPUT, 1,
			"Set 74HC595 output",
			"Params: <states (byte[])>",
			new string[] {
				"0x12 0x34 0x56 // 3 bytes",
				"0x34 0x56 0x78 // 3 bytes"
			},
			(command, parameters) =>
			{
				var data = Regex.Replace(parameters[0].Trim(), @"\s+", " ").Split(' ');
				var bytes = new byte[data.Length];
				for (int i = 0; i < data.Length; ++i)
					bytes[i] = _getTfromString<byte>(data[i]);

				var builder = new StringBuilder(bytes.Length * 3);
				foreach (var b in bytes)
					builder.AppendFormat(" {0:X2}", b);

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, length = {2}, data ={3}\r\n",
						DateTime.Now,
						command,
						data.Length,
						builder
					)
				);

				mCard.QuerySetOutput(bytes);
			}
		);
		mCommandProperty_WriteStorage = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_WRITE_STORAGE, 2,
			"Write data to the onboard storage.",
			"Params: <address (UInt16)>, <data (byte[])>",
			new string[] {
				"0x100, 0x12 0x34 0x56 0x78 0x90 0xAB 0xCD 0xEF",
				"0x200, 0xFE 0xDC 0xBA 0x09 0x87 0x65 0x43 0x21"
			},
			(command, parameters) =>
			{
				var address = _getTfromString<ushort>(parameters[0].Trim());
				var data = Regex.Replace(parameters[1].Trim(), @"\s+", " ").Split(' ');
				var bytes = new byte[data.Length];
				for (int i = 0; i < data.Length; ++i)
					bytes[i] = _getTfromString<byte>(data[i]);

				var builder = new StringBuilder(bytes.Length * 3);
				foreach (var b in bytes)
					builder.AppendFormat(" {0:X2}", b);

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, address = 0x{2:X4}, length = {3}, data ={4}\r\n",
						DateTime.Now,
						command,
						address,
						data.Length,
						builder
					)
				);

				mCard.QueryWriteStorage(address, bytes);
			}
		);
		mCommandProperty_ReadStorage = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_READ_STORAGE, 2,
			"Read data from onboard storage.",
			"Params: <address (UInt16)>, <length (byte)>",
			new string[] {
				//"0x000, 64 // read 64 bytes from 0x000",
				//"0x100, 8 // read 8 bytes from 0x100",
				//"0x200, 16 // read 16 bytes from 0x200",
				//"0x204, 4 // read 4 bytes from 0x204"
				"0x080, 64",
				"0x180, 64",
				"0x280, 64",
				"0x380, 64",
				"0x480, 64",
				"0x580, 64",
				"0x680, 64",
				"0x780, 64",
				"0x880, 64",
				"0x980, 64",
				"0xa80, 64",
				"0xb80, 64",
				"0xc80, 64"
			},
			(command, parameters) =>
			{
				var address = _getTfromString<ushort>(parameters[0].Trim());
				var length = _getTfromString<byte>(parameters[1].Trim());

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}, address = 0x{2:X4}, length = {3}\r\n",
						DateTime.Now,
						command,
						address,
						length
					)
				);

				mCard.QueryReadStorage(address, length);
			}
		);
		mCommandProperty_Reboot = new CommandProperty(
			on_error_callback,
			IOCard.Commands.CMD_REBOOT, 0,
			"Reboot the device",
			"Params: N/A",
			(command, parameters) =>
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						" => {0}: cmd = {1}\r\n",
						DateTime.Now,
						command
					)
				);

				mCard.QueueReboot();
			}
		);

		mCommandProperties = new CommandProperty[] {
			mCommandProperty_GetInfo,
			mCommandProperty_SetTrackLevel,
			mCommandProperty_EjectCoin,
			mCommandProperty_GetCoinCounter,
			mCommandProperty_ResetCoinCounter,
			mCommandProperty_TickAuditCounter,
			mCommandProperty_GetKeyMasks,
			mCommandProperty_GetKeys,
			mCommandProperty_SetOutputs,
			mCommandProperty_SetEjectTimeout,
			mCommandProperty_WriteStorage,
			mCommandProperty_ReadStorage,
			mCommandProperty_Reboot
		};

		var cmds = new List<string>(mCommandProperties.Length);
		foreach (var desc in mCommandProperties)
			cmds.Add(string.Format("0x{0:X2} ({1})", (uint)desc.Command, desc.Command.ToString().Replace("CMD_", "")));
		_populateComboBox(combobox_cmd, cmds);
		mLastCmdIndex = combobox_cmd.Active;
		label_cmd_desc.Text = mCommandProperties[mLastCmdIndex].CommandDescription;
		label_params_desc.Text = mCommandProperties[mLastCmdIndex].ParamsDescription;

		_populateComboBox(comboboxentry_port, mPorts);

		_populateComboBox(comboboxentry_params, mCommandProperties[mLastCmdIndex].HistoryParams);
		comboboxentry_params.Sensitive = mCommandProperties[mLastCmdIndex].Params != 0;
		if (mCommandProperties[mLastCmdIndex].HistoryParams.Count > 2)
		{
			label_params_history_1.Text = mCommandProperties[mLastCmdIndex].HistoryParams[1];
			button_send_1.Sensitive = true;
		}
		else {
			label_params_history_1.Text = "";
			button_send_1.Sensitive = false;
		}

		mCard.OnConnected += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				button_send.Sensitive = true;
				button_connect.Label = "_Disconnect";
			});
		};
		mCard.OnDisconnected += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				button_send.Sensitive = false;
				button_connect.Label = "_Connect";
			});
		};
		mCard.OnError += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				switch (e.ErrorCode)
				{
					case IOCard.Errors.ERR_EJECT_INTERRUPTED:
						{
							var ev = (IOCard.ErrorEjectInterruptedEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, Track = {2}, Coins Failed = {3}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.Track,
									ev.CoinsFailed
								)
							);
						}
						break;
					case IOCard.Errors.ERR_EJECT_TIMEOUT:
						{
							var ev = (IOCard.ErrorEjectTimeoutEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, Track = {2}, Coins Failed = {3}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.Track,
									ev.CoinsFailed
								)
							);
						}
						break;
					case IOCard.Errors.ERR_NOT_A_TRACK:
						{
							var ev = (IOCard.ErrorTrackEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, Track = {2}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.Track
								)
							);
						}
						break;
					case IOCard.Errors.ERR_NOT_A_COUNTER:
						{
							var ev = (IOCard.ErrorNotACounterEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, Counter = 0x{2:X2}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.AuditCounter
								)
							);
						}
						break;
					case IOCard.Errors.ERR_PROTECTED_STORAGE:
						{
							var ev = (IOCard.ErrorProtectedStorageEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, address = 0x{2:X4}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.Address
								)
							);
						}
						break;
					case IOCard.Errors.ERR_TOO_LONG:
						{
							var ev = (IOCard.ErrorTooLongEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, requested = {2}, desired = {3}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.RequestedLength,
									ev.DesiredLength
								)
							);
						}
						break;
					case IOCard.Errors.ERR_UNKNOWN_COMMAND:
						{
							var ev = (IOCard.ErrorUnknownCommandEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, cmd = 0x{2:X2}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.Command
								)
							);
						}
						break;
					case IOCard.Errors.ERR_UNKNOWN_ERROR:
						{
							var ev = (IOCard.ErrorUnknownErrorEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<=  {0}: error = {1}, unknown error = 0x{2:X2}\r\n",
									ev.DateTime,
									ev.ErrorCode,
									ev.UnknownErrorCode
								)
							);
						}
						break;
				}
			});
		};
		mCard.OnBoot += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Evt = {1}, Protocol = {2}\r\n",
						e.DateTime,
						e.GetType(),
						e.ProtocolVersion
					)
				);
			});
		};
		mCard.OnGetInfoResult += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Manufacturer = {1}, Product = {2}, Version = {3}, Protocol = {4}\r\n",
						e.DateTime,
						e.Manufacturer,
						e.Product,
						e.Version,
						e.ProtocolVersion
					)
				);
			});
		};
		mCard.OnCoinCounterResult += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Track = {1}, Coins = {2}\r\n",
						e.DateTime,
						e.Track,
						e.Coins
					)
				);
			});
		};
		mCard.OnKeys += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var builder = new StringBuilder(e.Keys.Length * 3);
				foreach (var key in e.Keys)
					builder.Append(string.Format(" {0:X2}", key));

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Keys:{1}\r\n",
						e.DateTime,
						builder
					)
				);
			});
		};
		mCard.OnKeyMasks += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var builder = new StringBuilder(e.KeyMasks.Length * 3);
				foreach (var mask in e.KeyMasks)
					builder.Append(string.Format(" {0:X2}", mask));

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Key Masks:{1}\r\n",
						e.DateTime,
						builder
					)
				);
			});
		};
		mCard.OnWriteStorageResult += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Address = 0x{1:X4}, Length = {2}\r\n",
						e.DateTime,
						e.Address,
						e.Length
					)
				);
			});
		};
		mCard.OnReadStorageResult += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var builder = new StringBuilder(e.Data.Length * 3);
				foreach (var data in e.Data)
					builder.Append(string.Format(" {0:X2}", data));

				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Address = 0x{1:X4}, Data ={2}\r\n",
						e.DateTime,
						e.Address,
						builder
					)
				);
			});
		};
		mCard.OnDebug += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Debug = {1}\r\n",
						e.DateTime,
						e.Message
					)
				);
			});
		};
		mCard.OnUnknown += (sender, e) =>
		{
			Application.Invoke(delegate
			{
				var iter = textview_received.Buffer.StartIter;
				textview_received.Buffer.Insert(
					ref iter,
					string.Format(
						"<=  {0}: Unknown = {1}\r\n",
						e.DateTime,
						e.Command.CommandString()
					)
				);
			});
		};
	}

	protected void OnDeleteEvent(object sender, DeleteEventArgs a)
	{
		mCard.Disconnect();
		Application.Quit();
		a.RetVal = true;
	}

	protected void OnComboBoxCmd_Changed(object sender, EventArgs e)
	{
		var cb = (ComboBox)sender;
		if (mLastCmdIndex == cb.Active)
			return;
		mLastCmdIndex = cb.Active;

		List<string> history = mCommandProperties[mLastCmdIndex].HistoryParams;
		_populateComboBox(comboboxentry_params, history);
		comboboxentry_params.Sensitive = mCommandProperties[mLastCmdIndex].Params != 0;
		label_cmd_desc.Text = mCommandProperties[mLastCmdIndex].CommandDescription;
		label_params_desc.Text = mCommandProperties[mLastCmdIndex].ParamsDescription;
		if (history.Count >= 2)
		{
			label_params_history_1.Text = history[1];
			button_send_1.Sensitive = mCard.IsConnected;
		}
		else {
			label_params_history_1.Text = "";
			button_send_1.Sensitive = false;
		}
	}

	protected void OnComboBoxEntryPort_Changed(object sender, EventArgs e)
	{
		var cbe = (ComboBoxEntry)sender;
		if (cbe.Active > 0)
		{
			string selected = cbe.ActiveText;
			mPorts.Remove(selected);
			mPorts.Insert(0, selected);
			_populateComboBox(comboboxentry_port, mPorts);
		}
	}

	protected void OnComboBoxEntryParams_Changed(object sender, EventArgs e)
	{
		var cbe = (ComboBoxEntry)sender;
		if (cbe.Active > 0)
		{
			var parameters = cbe.ActiveText;
			List<string> history = mCommandProperties[mLastCmdIndex].HistoryParams;
			history.Remove(parameters);
			history.Insert(0, parameters);
			_populateComboBox(comboboxentry_params, history);
			if (history.Count > 2)
			{
				label_params_history_1.Text = history[1];
				button_send_1.Sensitive = true;
			}
			else {
				label_params_history_1.Text = "";
				button_send_1.Sensitive = false;
			}
		}
	}

	protected void OnButtonConnect_Clicked(object sender, EventArgs e)
	{
		try
		{
			if (mCard.IsConnected)
			{
				Disconnect();
			}
			else
			{
				var port = comboboxentry_port.ActiveText;
				if (comboboxentry_port.Active != 0)
				{
					mPorts.Remove(port);
					mPorts.Insert(0, port);
					_populateComboBox(comboboxentry_port, mPorts);
				}

				Connect(port, 250000);
			}
		}
		catch (InvalidOperationException ex)
		{
			var iter = textview_received.Buffer.StartIter;
			textview_received.Buffer.Insert(
				ref iter,
				string.Format(
					"!!! {0}: Connect / Disconnect failed, reason = {1}\r\n",
					DateTime.Now,
					ex.Message
				)
			);
		}
	}

	protected void OnButtonSend_Clicked(object sender, EventArgs e)
	{
		var parameters_raw = comboboxentry_params.ActiveText;
		if (comboboxentry_params.Active != 0)
		{
			List<string> history = mCommandProperties[mLastCmdIndex].HistoryParams;
			history.Remove(parameters_raw);
			history.Insert(0, parameters_raw);
			_populateComboBox(comboboxentry_params, history);
		}

		parameters_raw = parameters_raw.Trim();
		var comment_idx = parameters_raw.IndexOf("//", StringComparison.Ordinal);
		parameters_raw = parameters_raw.Substring(0, comment_idx == -1 ? parameters_raw.Length : comment_idx).Trim();

		mCommandProperties[mLastCmdIndex].SendCommand(parameters_raw.Split(','));
	}

	protected void OnButtonSend1_Clicked(object sender, EventArgs e)
	{
		var parameters_raw = label_params_history_1.Text;
		List<string> history = mCommandProperties[mLastCmdIndex].HistoryParams;
		history.Remove(parameters_raw);
		history.Insert(0, parameters_raw);
		_populateComboBox(comboboxentry_params, history);

		parameters_raw = parameters_raw.Trim();
		var comment_idx = parameters_raw.IndexOf("//", StringComparison.Ordinal);
		parameters_raw = parameters_raw.Substring(0, comment_idx == -1 ? parameters_raw.Length : comment_idx).Trim();

		mCommandProperties[mLastCmdIndex].SendCommand(parameters_raw.Split(','));
	}

	void _populateComboBox(ComboBox cb, List<string> contents)
	{
		var store = (ListStore)cb.Model;
		store.Clear();
		foreach (var content in contents)
			store.AppendValues(content);
		cb.Active = contents.Count == 0 ? -1 : 0;
		if (cb is ComboBoxEntry && contents.Count == 0)
			((ComboBoxEntry)cb).Entry.Text = "";
	}

	static T _getTfromString<T>(string mystring)
	{
		var foo = TypeDescriptor.GetConverter(typeof(T));
		return (T)(foo.ConvertFromInvariantString(mystring));
	}

	#endregion
}