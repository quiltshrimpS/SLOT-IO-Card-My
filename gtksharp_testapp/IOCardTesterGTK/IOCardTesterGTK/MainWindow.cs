using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using Gtk;
using Spark.Slot.IO;

public partial class MainWindow : Window
{
	private static IOCard sCard = new IOCard();

	private int mLastCmdIndex = 0;

	private List<string> mPorts = new List<string>(new string[] {
		"COM3",
		"COM1",
		"COM20",
	});

	private class CommandProperties
	{
		public readonly IOCard.Commands Command;
		public readonly int Params;
		public readonly string CommandDescription;
		public readonly string ParamsDescription;
		public readonly List<string> HistoryParams;
		private readonly Action<IOCard.Commands, string[]> SendCommandCallback;

		public CommandProperties(IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc) :
			this(cmd, parameter_count, cmdDesc, paramsDesc, new string[0], (command, parameters) => { })
		{
		}

		public CommandProperties(IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc, string[] history) :
			this(cmd, parameter_count, cmdDesc, paramsDesc, history, (command, parameters) => { })
		{
		}

		public CommandProperties(IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc, Action<IOCard.Commands, string[]> callback) :
			this(cmd, parameter_count, cmdDesc, paramsDesc, new string[0], callback)
		{
		}

		public CommandProperties(IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc, string[] history, Action<IOCard.Commands, string[]> callback)
		{
			Command = cmd;
			Params = parameter_count;
			CommandDescription = cmdDesc;
			ParamsDescription = paramsDesc;
			HistoryParams = new List<string>(history);
			SendCommandCallback = callback;
		}

		public void SendCommand(string[] parameters)
		{
			SendCommandCallback.Invoke(Command, parameters);
		}
	}

	private CommandProperties[] sCommands = null;

	public MainWindow() : base(WindowType.Toplevel)
	{
		Build();

		Action<IOCard.Commands, string[]> unhandled_send_callback = (command, parameters) =>
		{
			var iter = textview_received.Buffer.StartIter;
			textview_received.Buffer.Insert(
				ref iter,
				string.Format(
					"=> {0}: unknown cmd = {1}, params = {2}\r\n",
					DateTime.Now,
					sCommands[mLastCmdIndex].Command,
					parameters.Length == 0 ? "<null>" : String.Join(", ", parameters)
				)
			);
		};

		sCommands = new CommandProperties[] {
			new CommandProperties(
				IOCard.Commands.CMD_GET_INFO, 0,
				"Get device information",
				"Params: N/A",
				(command, parameters) =>
				{
					var iter = textview_received.Buffer.StartIter;
					textview_received.Buffer.Insert(
						ref iter,
						string.Format(
							"=> {0}: cmd = {1}\r\n",
							DateTime.Now,
							sCommands[mLastCmdIndex].Command
						)
					);
					sCard.QueryGetInfo();
				}
			),
			new CommandProperties(
				IOCard.Commands.CMD_EJECT_COIN, 2,
				"Eject N coins.",
				"Params: <track (byte)>, <coins (byte)>",
				new string[] {
					"0xC0, 0x0A // eject 10 coins from track 0xC0 (eject track 1)",
					"0xC0, 5 // eject 5 coins from track 0xC0 (eject track 1)",
					"0xC0, 0 // interrupt track 0xC0 (eject track 1)",
				},
				(command, parameters) =>
				{
					var track = (IOCard.CoinTrack)(_getTfromString<byte>(parameters[0].Trim()));
					var count = _getTfromString<byte>(parameters[1].Trim());

					var iter = textview_received.Buffer.StartIter;
					textview_received.Buffer.Insert(
						ref iter,
						string.Format(
							"=> {0}: cmd = {1}, track = {2}, count = {3}\r\n",
							DateTime.Now,
							sCommands[mLastCmdIndex].Command,
							track,
							count
						)
					);

					sCard.QueryEjectCoin(track, count);
				}
			),
			new CommandProperties(
				IOCard.Commands.CMD_GET_COIN_COUNTER, 1,
				"Get coin counter.",
				"Params: <track (byte)>",
				new string[] {
					"0x00 // track 0x00 (insert 1)",
					"0x80 // track 0x80 (banknote 1)",
					"0xC0 // track 0xC0 (eject track 1)",
				},
				(command, parameters) =>
				{
					var track = (IOCard.CoinTrack)(_getTfromString<byte>(parameters[0].Trim()));

					var iter = textview_received.Buffer.StartIter;
					textview_received.Buffer.Insert(
						ref iter,
						string.Format(
							"=> {0}: cmd = {1}, track = {2}\r\n",
							DateTime.Now,
							sCommands[mLastCmdIndex].Command,
							track
						)
					);

					sCard.QueryGetCoinCounter(track);
				}
			),
			new CommandProperties(
				IOCard.Commands.CMD_GET_KEYS, 0,
				"Get key states from device",
				"Params: N/A",
				(command, parameters) =>
				{
					var iter = textview_received.Buffer.StartIter;
					textview_received.Buffer.Insert(
						ref iter,
						string.Format(
							"=> {0}: cmd = {1}\r\n",
							DateTime.Now,
							sCommands[mLastCmdIndex].Command
						)
					);
					sCard.QueryGetKeys();
				}
			),
			new CommandProperties(
				IOCard.Commands.CMD_RESET_COIN_COINTER, 1,
				"Reset coin counter.",
				"Params: <track (byte)>",
				new string[] {
					"0x00 // track 0x00 (insert 1)",
					"0x80 // track 0x80 (banknote 1)",
					"0xC0 // track 0xC0 (eject track 1)",
				},
				(command, parameters) =>
				{
					var track = (IOCard.CoinTrack)(_getTfromString<byte>(parameters[0].Trim()));

					var iter = textview_received.Buffer.StartIter;
					textview_received.Buffer.Insert(
						ref iter,
						string.Format(
							"=> {0}: cmd = {1}, track = {2}\r\n",
							DateTime.Now,
							sCommands[mLastCmdIndex].Command,
							track
						)
					);

					sCard.QueryResetCoinCounter(track);
				}
			),
			new CommandProperties(
				IOCard.Commands.CMD_SET_OUTPUT, 1,
				"Set 74HC595 output.",
				"Params: <states (byte[])>",
				new string[] {
					"0x12 0x34 0x56 // 3 bytes",
					"0x34 0x56 0x78 // 3 bytes",
				},
				unhandled_send_callback
			),
			new CommandProperties(
				IOCard.Commands.CMD_WRITE_STORAGE, 2,
				"Write data to the onboard storage.",
				"Params: <address (UInt16)>, <data (byte[])>",
				new string[] {
					"0x100, 0x12 0x34 0x56 0x78 0x90 0xAB 0xCD 0xEF",
					"0x200, 0xFE 0xDC 0xBA 0x09 0x87 0x65 0x43 0x21",
				},
				unhandled_send_callback
			),
			new CommandProperties(
				IOCard.Commands.CMD_READ_STORAGE, 2,
				"Read data from onboard storage.",
				"Params: <address (UInt16)>, <length (byte)>",
				new string[] {
					"0x100, 8 // read 8 bytes from 0x100",
					"0x200, 4 // read 4 bytes from 0x200",
					"0x204, 4 // read 4 bytes from 0x204",
				},
				unhandled_send_callback
			),
		};

		var cmds = new List<string>(sCommands.Length);
		foreach (var desc in sCommands)
			cmds.Add(string.Format(
				"{0} ({1})",
				// FIXME: because Mono doesn't read {0:X2}, so we had do it this way.
				string.Format("0x{0:x}", desc.Command).Replace("0x000000", "0x"),
				desc.Command.ToString().Replace("CMD_", "")
			));
		_populateComboBox(combobox_cmd, cmds);
		mLastCmdIndex = combobox_cmd.Active;
		label_cmd_desc.Text = sCommands[mLastCmdIndex].CommandDescription;
		label_params_desc.Text = sCommands[mLastCmdIndex].ParamsDescription;

		_populateComboBoxEntry(comboboxentry_params, sCommands[mLastCmdIndex].HistoryParams);
		comboboxentry_params.Sensitive = sCommands[mLastCmdIndex].Params != 0;
		_populateComboBoxEntry(comboboxentry_port, mPorts);

		sCard.OnConnected += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				button_send.Sensitive = true;
				button_connect.Label = "_Disconnect";
			});
		};
		sCard.OnDisconnected += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				button_send.Sensitive = false;
				button_connect.Label = "_Connect";
			});
		};
		sCard.OnError += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
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
									"<= {0}: error = {1}, Track = 0x{2:X}, Coins Failed = {3}\r\n",
									DateTime.Now,
									ev.ErrorCode,
									ev.Track,
									ev.CoinsFailed
								)
							);
						}
						break;
					case IOCard.Errors.ERR_EJECT_TIMEOUT:
					case IOCard.Errors.ERR_NOT_A_TRACK:
						{
							var ev = (IOCard.ErrorTrackEventArgs)e;
							var iter = textview_received.Buffer.StartIter;
							textview_received.Buffer.Insert(
								ref iter,
								string.Format(
									"<= {0}: error = {1}, Track = 0x{2:X}\r\n",
									DateTime.Now,
									ev.ErrorCode,
									ev.Track
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
									"<= {0}: error = {1}, cmd = 0x{2:X}\r\n",
									DateTime.Now,
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
									"<= {0}: error = {1}, unknown error = 0x{2:X}\r\n",
									DateTime.Now,
									ev.ErrorCode,
									ev.UnknownErrorCode
								)
							);
						}
						break;
				}
			});
		};
		sCard.OnGetInfoResult += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				textview_received.Buffer.Text = string.Format(
					"<= {0}: Manufacturer = {1}, Product = {2}, Version = {3}, Protocol = {4}\r\n{5}",
					DateTime.Now,
					e.Manufacturer,
					e.Product,
					e.Version,
					e.ProtocolVersion,
					textview_received.Buffer.Text
				);
			});
		};
		sCard.OnCoinCounterResult += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				textview_received.Buffer.Text = string.Format(
					"<= {0}: Track = {1}, Coins = {2}\r\n{3}",
					DateTime.Now,
					e.Track,
					e.Coins,
					textview_received.Buffer.Text
				);
			});
		};
		sCard.OnKey += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				var builder = new StringBuilder(e.Keys.Length * 5);
				foreach (var key in e.Keys)
					builder.Append(" 0x").AppendFormat("{0:X}", key);
				textview_received.Buffer.Text = string.Format(
					"<= {0}: Keys:{1}\r\n{2}",
					DateTime.Now,
					builder,
					textview_received.Buffer.Text
				);
			});
		};
		sCard.OnUnknown += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				textview_received.Buffer.Text = string.Format(
					"<= {0}: Unknown - {1}\r\n{2}",
					DateTime.Now,
					e.Command.CommandString(),
					textview_received.Buffer.Text
				);
			});
		};
	}

	protected void OnDeleteEvent(object sender, DeleteEventArgs a)
	{
		sCard.Disconnect();
		Application.Quit();
		a.RetVal = true;
	}

	protected void OnComboBoxCmd_Changed(object sender, EventArgs e)
	{
		var cb = (ComboBox)sender;
		if (mLastCmdIndex == cb.Active)
			return;
		mLastCmdIndex = cb.Active;

		_populateComboBoxEntry(comboboxentry_params, sCommands[mLastCmdIndex].HistoryParams);
		comboboxentry_params.Sensitive = sCommands[mLastCmdIndex].Params != 0;
		label_cmd_desc.Text = sCommands[mLastCmdIndex].CommandDescription;
		label_params_desc.Text = sCommands[mLastCmdIndex].ParamsDescription;
	}

	protected void OnComboBoxEntryPort_Changed(object sender, EventArgs e)
	{
		var cbe = (ComboBoxEntry)sender;
		if (cbe.Active > 0)
		{
			string selected = cbe.ActiveText;
			mPorts.Remove(selected);
			mPorts.Insert(0, selected);
			_populateComboBoxEntry(comboboxentry_port, mPorts);
		}
	}

	protected void OnComboBoxEntryParams_Changed(object sender, EventArgs e)
	{
		var cbe = (ComboBoxEntry)sender;
		if (cbe.Active > 0)
		{
			var parameters = cbe.ActiveText;
			sCommands[mLastCmdIndex].HistoryParams.Remove(parameters);
			sCommands[mLastCmdIndex].HistoryParams.Insert(0, parameters);
			_populateComboBoxEntry(comboboxentry_params, sCommands[mLastCmdIndex].HistoryParams);
		}
	}

	protected void OnButtonConnect_Clicked(object sender, EventArgs e)
	{
		if (sCard.IsConnected)
		{
			sCard.Disconnect();
		}
		else
		{
			var port = comboboxentry_port.ActiveText;
			if (comboboxentry_port.Active != 0)
			{
				mPorts.Remove(port);
				mPorts.Insert(0, port);
				_populateComboBoxEntry(comboboxentry_port, mPorts);
			}
			var status = sCard.Connect(port, 250000);
		}
	}

	protected void OnButtonSend_Clicked(object sender, EventArgs e)
	{
		var parameters_raw = comboboxentry_params.ActiveText;
		if (comboboxentry_params.Active != 0)
		{
			sCommands[mLastCmdIndex].HistoryParams.Remove(parameters_raw);
			sCommands[mLastCmdIndex].HistoryParams.Insert(0, parameters_raw);
			_populateComboBoxEntry(comboboxentry_params, sCommands[mLastCmdIndex].HistoryParams);
		}

		parameters_raw = parameters_raw.Trim();
		var comment_idx = parameters_raw.IndexOf("//", StringComparison.Ordinal);
		parameters_raw = parameters_raw.Substring(0, comment_idx == -1 ? parameters_raw.Length : comment_idx).Trim();

		sCommands[mLastCmdIndex].SendCommand(parameters_raw.Split(','));
	}

	private void _populateComboBox(ComboBox cb, List<string> contents)
	{
		var store = (ListStore)cb.Model;
		store.Clear();
		foreach (var content in contents)
			store.AppendValues(content);
		cb.Active = contents.Count == 0 ? -1 : 0;
	}

	private void _populateComboBoxEntry(ComboBoxEntry cbe, List<string> contents)
	{
		_populateComboBox(cbe, contents);
		if (contents.Count == 0)
			cbe.Entry.Text = "";
	}

	private static T _getTfromString<T>(string mystring)
	{
		var foo = TypeDescriptor.GetConverter(typeof(T));
		return (T)(foo.ConvertFromInvariantString(mystring));
	}
}