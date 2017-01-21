using System;
using System.Collections.Generic;
using Gtk;

public partial class MainWindow : Gtk.Window
{
	private int mLastCmdIndex = 0;
	private List<string> mPorts = new List<string>();
	private Dictionary<int, List<string>> mPayloads = new Dictionary<int, List<string>>();

	public MainWindow() : base(Gtk.WindowType.Toplevel)
	{
		Build();
		mLastCmdIndex = combobox_cmd.Active;

		if (!mPayloads.ContainsKey(mLastCmdIndex))
			mPayloads.Add(0, new List<string>());
		mPayloads[0].Add("blah1");
		mPayloads[0].Add("blah2");
		mPayloads[0].Add("blah3");
		mPayloads[0].Add("blah4");
		mPayloads[0].Add("blah5");

		_populateComboBoxEntryPayload(mLastCmdIndex);
		_populateComboBoxEntryPort();
	}

	protected void OnDeleteEvent(object sender, DeleteEventArgs a)
	{
		Application.Quit();
		a.RetVal = true;
	}

	protected void OnComboBoxCmd_Changed(object sender, EventArgs e)
	{
		var cb = (Gtk.ComboBox)sender;
		if (mLastCmdIndex == cb.Active)
			return;
		mLastCmdIndex = cb.Active;

		_populateComboBoxEntryPayload(mLastCmdIndex);
	}

	protected void OnComboBoxEntryPort_Changed(object sender, EventArgs e)
	{
		var cbe = (Gtk.ComboBoxEntry)sender;
		if (cbe.Active > 0)
		{
			string selected = cbe.ActiveText;
			mPorts.Remove(selected);
			mPorts.Insert(0, selected);
			_populateComboBoxEntryPort();
		}
	}

	protected void OnComboBoxEntryPayload_Changed(object sender, EventArgs e)
	{
		var cbe = (Gtk.ComboBoxEntry)sender;
		if (cbe.Active > 0)
		{
			var payload = cbe.ActiveText;
			var payloads = mPayloads[mLastCmdIndex];
			payloads.Remove(payload);
			payloads.Insert(0, payload);
			_populateComboBoxEntryPayload(mLastCmdIndex);
		}
	}

	protected void OnButtonConnect_Clicked(object sender, EventArgs e)
	{
		if (comboboxentry_port.Active != 0)
		{
			string port = comboboxentry_port.ActiveText;
			mPorts.Remove(port);
			mPorts.Insert(0, port);
			_populateComboBoxEntryPort();
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
			_populateComboBoxEntryPayload(mLastCmdIndex);
		}
	}

	private void _populateComboBoxEntryPort()
	{
		var store = (Gtk.ListStore)comboboxentry_port.Model;
		store.Clear();
		foreach (var port in mPorts)
			store.AppendValues(port);
		comboboxentry_port.Active = 0;
	}

	private void _populateComboBoxEntryPayload(int index)
	{
		if (!mPayloads.ContainsKey(index))
			mPayloads.Add(mLastCmdIndex, new List<string>());
		var payloads = mPayloads[index];
		var store = (Gtk.ListStore)comboboxentry_payload.Model;
		store.Clear();
		foreach (var payload in payloads)
			store.AppendValues(payload);
		comboboxentry_payload.Active = 0;
	}
}