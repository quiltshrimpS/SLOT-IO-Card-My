using System;
using System.Collections.Generic;
using Gtk;
using Spark.Slot.IO;

public partial class MainWindow : Window
{
	private int mLastCmdIndex = 0;

	private List<string> mPorts = new List<string>();

	private struct CommandDescription
	{
		public IOCard.Commands Command;
		public bool HasPayload;
		public string Description;

		public CommandDescription(IOCard.Commands cmd, bool hasPayload, string desc)
		{
			Command = cmd;
			HasPayload = hasPayload;
			Description = desc;
		}
	}

	private static CommandDescription[] sCommands = new CommandDescription[]
	{
		new CommandDescription(IOCard.Commands.CMD_GET_INFO, false,
			"Get the device information (model / version)"),
		new CommandDescription(IOCard.Commands.CMD_EJECT_COIN, true,
		    "Eject N coins. Payload: [coin (1 byte)]"),
		new CommandDescription(IOCard.Commands.CMD_GET_COIN_COUNTER, false,
		    "Get coin counter. Payload: [trackId (1 byte)]"),
		new CommandDescription(IOCard.Commands.CMD_RESET_COIN_COINTER, false,
		    "Reset coin counter. Payload: [trackId (1 byte)]"),
		new CommandDescription(IOCard.Commands.CMD_SET_OUTPUT, false,
		    "Set 74HC595 output. Payload: [State (N bytes)], length automatically calculated."),
		new CommandDescription(IOCard.Commands.CMD_WRITE_STORAGE, false,
		    "Write data to the onboard storage. Payload: [address (2 bytes)][data (N bytes)], length automatically calculated."),
		new CommandDescription(IOCard.Commands.CMD_READ_STORAGE, false,
		    "Read data from onboard storage. Payload: [address (2 bytes)][length (1 byte)]"),
	};

	private Dictionary<int, List<string>> mPayloads = new Dictionary<int, List<string>>();

	public MainWindow() : base(WindowType.Toplevel)
	{
		Build();
		mLastCmdIndex = combobox_cmd.Active;

		mPayloads.Add(0, new List<string>());
		mPayloads[0].Add("blah1");
		mPayloads[0].Add("blah2");
		mPayloads[0].Add("blah3");
		mPayloads[0].Add("blah4");
		mPayloads[0].Add("blah5");

		mPayloads.Add(2, new List<string>());
		mPayloads[2].Add("blah6");
		mPayloads[2].Add("blah7");
		mPayloads[2].Add("blah8");
		mPayloads[2].Add("blah9");
		mPayloads[2].Add("blah10");

		if (!mPayloads.ContainsKey(mLastCmdIndex))
			mPayloads.Add(mLastCmdIndex, new List<string>());
		_populateComboBoxEntry(comboboxentry_payload, mPayloads[mLastCmdIndex]);
		_populateComboBoxEntry(comboboxentry_port, mPorts);

		var cmds = new List<string>(sCommands.Length);
		foreach (var desc in sCommands)
			cmds.Add(string.Format(
				"{0} ({1})",
				// FIXME: because Mono doesn't read {0:X2}, so we had do it this way.
				string.Format("0x{0:x}", desc.Command).Replace("0x000000", "0x"),
				desc.Command.ToString().Replace("CMD_", "")
			));
		_populateComboBox(combobox_cmd, cmds);
		label_cmd_desc.Text = sCommands[mLastCmdIndex].Description;
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

		if (!mPayloads.ContainsKey(mLastCmdIndex))
			mPayloads.Add(mLastCmdIndex, new List<string>());
		_populateComboBoxEntry(comboboxentry_payload, mPayloads[mLastCmdIndex]);

		label_cmd_desc.Text = sCommands[mLastCmdIndex].Description;
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

	protected void OnComboBoxEntryPayload_Changed(object sender, EventArgs e)
	{
		var cbe = (ComboBoxEntry)sender;
		if (cbe.Active > 0)
		{
			var payload = cbe.ActiveText;
			var payloads = mPayloads[mLastCmdIndex];
			payloads.Remove(payload);
			payloads.Insert(0, payload);

			if (!mPayloads.ContainsKey(mLastCmdIndex))
				mPayloads.Add(mLastCmdIndex, new List<string>());
			_populateComboBoxEntry(comboboxentry_payload, mPayloads[mLastCmdIndex]);
		}
	}

	protected void OnButtonConnect_Clicked(object sender, EventArgs e)
	{
		if (comboboxentry_port.Active != 0)
		{
			string port = comboboxentry_port.ActiveText;
			mPorts.Remove(port);
			mPorts.Insert(0, port);
			_populateComboBoxEntry(comboboxentry_port, mPorts);
		}
	}

	protected void OnButtonSend_Clicked(object sender, EventArgs e)
	{
		if (comboboxentry_payload.Active != 0)
		{
			var payload = comboboxentry_payload.ActiveText;
			var payloads = mPayloads[mLastCmdIndex];
			payloads.Remove(payload);
			payloads.Insert(0, payload);

			if (!mPayloads.ContainsKey(mLastCmdIndex))
				mPayloads.Add(mLastCmdIndex, new List<string>());
			_populateComboBoxEntry(comboboxentry_payload, mPayloads[mLastCmdIndex]);
		}
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