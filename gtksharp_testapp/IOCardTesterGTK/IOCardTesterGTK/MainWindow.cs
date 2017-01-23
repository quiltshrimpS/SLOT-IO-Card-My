using System;
using System.Collections.Generic;
using System.ComponentModel;
using Gtk;
using Spark.Slot.IO;

public partial class MainWindow : Window
{
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

	private static CommandProperties[] sCommands = {
		new CommandProperties(
			IOCard.Commands.CMD_GET_INFO, 0,
			"Get device information",
			"Params: N/A",
			(command, parameters) => {
				IOCard.Card.QueryGetInfo();
			}
		),
		new CommandProperties(
			IOCard.Commands.CMD_EJECT_COIN, 2,
			"Eject N coins.",
			"Params: <track (byte)>, <coins (byte)>",
			new string[] {
				"0x80, 0x0A // eject 10 coins from track 0x80",
				"0x80, 010 // eject 8 coins from track 0x80",
				"0x80, 0 // interrupt track 0x80",
			},
			(command, parameters) => {
				var track = (IOCard.CoinTrack)(_getTfromString<byte>(parameters[0].Trim()));
				var count = _getTfromString<byte>(parameters[1].Trim());
				IOCard.Card.QueryEjectCoin(track, count);
			}
		),
		new CommandProperties(
			IOCard.Commands.CMD_GET_COIN_COUNTER, 1,
			"Get coin counter.",
			"Params: <track (byte)>",
			new string[] {
				"0x00 // track 0x00 (insert 1)",
				"0x80 // track 0x80 (eject track)",
			},
			(command, parameters) => {
				var track = (IOCard.CoinTrack)(_getTfromString<byte>(parameters[0].Trim()));
				IOCard.Card.QueryGetCoinCounter(track);
			}
		),
		new CommandProperties(
			IOCard.Commands.CMD_RESET_COIN_COINTER, 1,
			"Reset coin counter.",
			"Params: <track (byte)>",
			new string[] { "0x80 // track 0x80" }
		),
		new CommandProperties(
			IOCard.Commands.CMD_SET_OUTPUT, 1,
			"Set 74HC595 output.",
			"Params: <states (byte[])>",
			new string[] {
				"0x12 0x34 0x56 // 3 bytes",
				"0x34 0x56 0x78 // 3 bytes",
			}
		),
		new CommandProperties(
			IOCard.Commands.CMD_WRITE_STORAGE, 2,
			"Write data to the onboard storage.",
			"Params: <address (UInt16)>, <data (byte[])>",
			new string[] {
				"0x100, 0x12 0x34 0x56 0x78 0x90 0xAB 0xCD 0xEF",
				"0x200, 0xFE 0xDC 0xBA 0x09 0x87 0x65 0x43 0x21",
			}
		),
		new CommandProperties(
			IOCard.Commands.CMD_READ_STORAGE, 2,
			"Read data from onboard storage.",
			"Params: <address (UInt16)>, <length (byte)>",
			new string[] {
				"0x100, 8 // read 8 bytes from 0x100",
				"0x200, 4 // read 4 bytes from 0x200",
				"0x204, 4 // read 4 bytes from 0x204",
			}
		),
	};

	public MainWindow() : base(WindowType.Toplevel)
	{
		Build();

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

		IOCard.Card.OnConnected += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				button_send.Sensitive = true;
				button_connect.Label = "_Disconnect";
			});
		};
		IOCard.Card.OnDisconnected += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				button_send.Sensitive = false;
				button_connect.Label = "_Connect";
			});
		};
		IOCard.Card.OnError += (sender, e) =>
		{
			Gtk.Application.Invoke(delegate
			{
				switch (e.ErrorCode)
				{
					case IOCard.Errors.ERR_EJECT_INTERRUPTED:
						{
							var ev = (IOCard.ErrorEjectInterruptedEventArgs)e;
							textview_received.Buffer.Text = string.Format(
								"<= {0}: {1} - Track = {2}, Coins failed = {3}\r\n{4}",
								DateTime.Now,
								ev.ErrorCode,
								ev.Track,
								ev.CoinsFailed,
								textview_received.Buffer.Text
							);
						}
						break;
					case IOCard.Errors.ERR_EJECT_TIMEOUT:
					case IOCard.Errors.ERR_NOT_A_TRACK:
						{
							var ev = (IOCard.ErrorTrackEventArgs)e;
							textview_received.Buffer.Text = string.Format(
								"<= {0}: {1} - Track = {2}\r\n{3}",
								DateTime.Now,
								ev.ErrorCode,
								ev.Track,
								textview_received.Buffer.Text
							);
						}
						break;
					case IOCard.Errors.ERR_UNKNOWN_COMMAND:
						{
							var ev = (IOCard.ErrorUnknownCommandEventArgs)e;
							textview_received.Buffer.Text = string.Format(
								"<= {0}: {1} - CommandID = 0x{2:X}\r\n{3}",
								DateTime.Now,
								ev.ErrorCode,
								ev.Command,
								textview_received.Buffer.Text
							);
						}
						break;
					case IOCard.Errors.ERR_UNKNOWN_ERROR:
						{
							var ev = (IOCard.ErrorUnknownErrorEventArgs)e;
							textview_received.Buffer.Text = string.Format(
								"<= {0}: {1} - Unknown error code = 0x{2:X}\r\n{3}",
								DateTime.Now,
								ev.ErrorCode,
								ev.UnknownErrorCode,
								textview_received.Buffer.Text
							);
						}
						break;
				}
			});
		};
		IOCard.Card.OnGetInfoResult += (sender, e) =>
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
		IOCard.Card.OnCoinCounterResult += (sender, e) =>
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
		IOCard.Card.OnUnknown += (sender, e) =>
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
		IOCard.Card.Disconnect();
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
		if (IOCard.Card.IsConnected)
		{
			IOCard.Card.Disconnect();
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
			var status = IOCard.Card.Connect(port, 250000);
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

		textview_received.Buffer.Text = string.Format(
			"=> {0}: cmd = {1}, params = {2}\r\n{3}",
			DateTime.Now,
			sCommands[mLastCmdIndex].Command,
			parameters_raw.Length == 0 ? "<null>" : parameters_raw,
			textview_received.Buffer.Text
		);

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