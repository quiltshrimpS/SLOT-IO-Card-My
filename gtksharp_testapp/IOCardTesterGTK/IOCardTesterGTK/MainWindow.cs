using System;
using System.Collections.Generic;
using Gtk;
using Spark.Slot.IO;

public partial class MainWindow : Window
{
	private int mLastCmdIndex = 0;

	private List<string> mPorts = new List<string>(new string[] {
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

		public CommandProperties(IOCard.Commands cmd, int parameter_count, string cmdDesc, string paramsDesc, string[] history)
		{
			Command = cmd;
			Params = parameter_count;
			CommandDescription = cmdDesc;
			ParamsDescription = paramsDesc;
			HistoryParams = new List<string>(history);
		}
	}

	private static CommandProperties[] sCommands = {
		new CommandProperties(
			IOCard.Commands.CMD_GET_INFO, 0,
			"Get device information",
			"Params: N/A",
			new string[0]
		),
		new CommandProperties(
			IOCard.Commands.CMD_EJECT_COIN, 2,
			"Eject N coins.",
			"Params: <track (byte)>, <coins (byte)>",
			new string[] {
				"0x80, 0x0A // eject 10 coins from track 0x80",
				"0x80, 010 // eject 8 coins from track 0x80",
				"0x80, 0 // interrupt track 0x80",
			}
		),
		new CommandProperties(
			IOCard.Commands.CMD_GET_COIN_COUNTER, 1,
			"Get coin counter.",
			"Params: <track (byte)>",
			new string[] { "0x80 // track 0x80" }
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
		mLastCmdIndex = combobox_cmd.Active;

		var cmds = new List<string>(sCommands.Length);
		foreach (var desc in sCommands)
			cmds.Add(string.Format(
				"{0} ({1})",
				// FIXME: because Mono doesn't read {0:X2}, so we had do it this way.
				string.Format("0x{0:x}", desc.Command).Replace("0x000000", "0x"),
				desc.Command.ToString().Replace("CMD_", "")
			));
		_populateComboBox(combobox_cmd, cmds);
		label_cmd_desc.Text = sCommands[mLastCmdIndex].CommandDescription;
		label_params_desc.Text = sCommands[mLastCmdIndex].ParamsDescription;

		_populateComboBoxEntry(comboboxentry_params, sCommands[mLastCmdIndex].HistoryParams);
		comboboxentry_params.Sensitive = sCommands[mLastCmdIndex].Params != 0;
		_populateComboBoxEntry(comboboxentry_port, mPorts);

		IOCard.Card.OnConnected += (sender, e) =>
		{
			button_send.Sensitive = true;
			button_connect.Label = "_Disconnect";
		};
		IOCard.Card.OnDisconnected += (sender, e) =>
		{
			button_send.Sensitive = false;
			button_connect.Label = "_Connect";
		};
	}

	protected void OnDeleteEvent(object sender, DeleteEventArgs a)
	{
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
			var status = IOCard.Card.Connect(port);
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
		parameters_raw = parameters_raw.Substring(0, comment_idx == -1 ? parameters_raw.Length : comment_idx);

		textview_received.Buffer.Text =
			string.Format("{0}: {1}\r\n", DateTime.Now, parameters_raw) +
			textview_received.Buffer.Text;
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
}