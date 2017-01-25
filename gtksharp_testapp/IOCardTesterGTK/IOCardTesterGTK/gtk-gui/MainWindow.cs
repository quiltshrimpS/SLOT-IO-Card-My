
// This file has been generated by the GUI designer. Do not modify.

public partial class MainWindow
{
	private global::Gtk.Table table1;

	private global::Gtk.Button button_connect;

	private global::Gtk.Button button_send;

	private global::Gtk.ComboBox combobox_cmd;

	private global::Gtk.ComboBoxEntry comboboxentry_params;

	private global::Gtk.ComboBoxEntry comboboxentry_port;

	private global::Gtk.ScrolledWindow GtkScrolledWindow;

	private global::Gtk.TextView textview_received;

	private global::Gtk.Label label_cmd;

	private global::Gtk.Label label_cmd_desc;

	private global::Gtk.Label label_params;

	private global::Gtk.Label label_params_desc;

	private global::Gtk.Label label_port;

	protected virtual void Build()
	{
		global::Stetic.Gui.Initialize(this);
		// Widget MainWindow
		this.Name = "MainWindow";
		this.Title = global::Mono.Unix.Catalog.GetString("SLOT-IO-Card Demo App");
		this.WindowPosition = ((global::Gtk.WindowPosition)(4));
		// Container child MainWindow.Gtk.Container+ContainerChild
		this.table1 = new global::Gtk.Table(((uint)(4)), ((uint)(5)), false);
		this.table1.Name = "table1";
		this.table1.RowSpacing = ((uint)(6));
		this.table1.ColumnSpacing = ((uint)(6));
		this.table1.BorderWidth = ((uint)(6));
		// Container child table1.Gtk.Table+TableChild
		this.button_connect = new global::Gtk.Button();
		this.button_connect.CanFocus = true;
		this.button_connect.Name = "button_connect";
		this.button_connect.UseUnderline = true;
		this.button_connect.Label = global::Mono.Unix.Catalog.GetString("_Connect");
		this.table1.Add(this.button_connect);
		global::Gtk.Table.TableChild w1 = ((global::Gtk.Table.TableChild)(this.table1[this.button_connect]));
		w1.LeftAttach = ((uint)(4));
		w1.RightAttach = ((uint)(5));
		w1.XOptions = ((global::Gtk.AttachOptions)(4));
		w1.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.button_send = new global::Gtk.Button();
		this.button_send.Sensitive = false;
		this.button_send.CanFocus = true;
		this.button_send.Name = "button_send";
		this.button_send.UseUnderline = true;
		this.button_send.Label = global::Mono.Unix.Catalog.GetString("_Send");
		this.table1.Add(this.button_send);
		global::Gtk.Table.TableChild w2 = ((global::Gtk.Table.TableChild)(this.table1[this.button_send]));
		w2.TopAttach = ((uint)(1));
		w2.BottomAttach = ((uint)(2));
		w2.LeftAttach = ((uint)(4));
		w2.RightAttach = ((uint)(5));
		w2.XOptions = ((global::Gtk.AttachOptions)(4));
		w2.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.combobox_cmd = global::Gtk.ComboBox.NewText();
		this.combobox_cmd.CanDefault = true;
		this.combobox_cmd.Events = ((global::Gdk.EventMask)(65536));
		this.combobox_cmd.Name = "combobox_cmd";
		this.table1.Add(this.combobox_cmd);
		global::Gtk.Table.TableChild w3 = ((global::Gtk.Table.TableChild)(this.table1[this.combobox_cmd]));
		w3.TopAttach = ((uint)(1));
		w3.BottomAttach = ((uint)(2));
		w3.LeftAttach = ((uint)(1));
		w3.RightAttach = ((uint)(2));
		w3.XOptions = ((global::Gtk.AttachOptions)(0));
		w3.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.comboboxentry_params = global::Gtk.ComboBoxEntry.NewText();
		this.comboboxentry_params.Name = "comboboxentry_params";
		this.table1.Add(this.comboboxentry_params);
		global::Gtk.Table.TableChild w4 = ((global::Gtk.Table.TableChild)(this.table1[this.comboboxentry_params]));
		w4.TopAttach = ((uint)(1));
		w4.BottomAttach = ((uint)(2));
		w4.LeftAttach = ((uint)(3));
		w4.RightAttach = ((uint)(4));
		w4.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.comboboxentry_port = global::Gtk.ComboBoxEntry.NewText();
		this.comboboxentry_port.Name = "comboboxentry_port";
		this.table1.Add(this.comboboxentry_port);
		global::Gtk.Table.TableChild w5 = ((global::Gtk.Table.TableChild)(this.table1[this.comboboxentry_port]));
		w5.LeftAttach = ((uint)(1));
		w5.RightAttach = ((uint)(4));
		w5.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.GtkScrolledWindow = new global::Gtk.ScrolledWindow();
		this.GtkScrolledWindow.Name = "GtkScrolledWindow";
		this.GtkScrolledWindow.ShadowType = ((global::Gtk.ShadowType)(1));
		// Container child GtkScrolledWindow.Gtk.Container+ContainerChild
		this.textview_received = new global::Gtk.TextView();
		this.textview_received.CanFocus = true;
		this.textview_received.Name = "textview_received";
		this.textview_received.Editable = false;
		this.textview_received.AcceptsTab = false;
		this.GtkScrolledWindow.Add(this.textview_received);
		this.table1.Add(this.GtkScrolledWindow);
		global::Gtk.Table.TableChild w7 = ((global::Gtk.Table.TableChild)(this.table1[this.GtkScrolledWindow]));
		w7.TopAttach = ((uint)(3));
		w7.BottomAttach = ((uint)(4));
		w7.RightAttach = ((uint)(5));
		w7.XOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.label_cmd = new global::Gtk.Label();
		this.label_cmd.Name = "label_cmd";
		this.label_cmd.Xalign = 0F;
		this.label_cmd.LabelProp = global::Mono.Unix.Catalog.GetString("Cmd:");
		this.table1.Add(this.label_cmd);
		global::Gtk.Table.TableChild w8 = ((global::Gtk.Table.TableChild)(this.table1[this.label_cmd]));
		w8.TopAttach = ((uint)(1));
		w8.BottomAttach = ((uint)(2));
		w8.XOptions = ((global::Gtk.AttachOptions)(0));
		w8.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.label_cmd_desc = new global::Gtk.Label();
		this.label_cmd_desc.Name = "label_cmd_desc";
		this.label_cmd_desc.Xalign = 0F;
		this.table1.Add(this.label_cmd_desc);
		global::Gtk.Table.TableChild w9 = ((global::Gtk.Table.TableChild)(this.table1[this.label_cmd_desc]));
		w9.TopAttach = ((uint)(2));
		w9.BottomAttach = ((uint)(3));
		w9.RightAttach = ((uint)(2));
		w9.XOptions = ((global::Gtk.AttachOptions)(6));
		w9.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.label_params = new global::Gtk.Label();
		this.label_params.Name = "label_params";
		this.label_params.Xalign = 0F;
		this.label_params.LabelProp = global::Mono.Unix.Catalog.GetString("Params:");
		this.table1.Add(this.label_params);
		global::Gtk.Table.TableChild w10 = ((global::Gtk.Table.TableChild)(this.table1[this.label_params]));
		w10.TopAttach = ((uint)(1));
		w10.BottomAttach = ((uint)(2));
		w10.LeftAttach = ((uint)(2));
		w10.RightAttach = ((uint)(3));
		w10.XOptions = ((global::Gtk.AttachOptions)(2));
		w10.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.label_params_desc = new global::Gtk.Label();
		this.label_params_desc.Name = "label_params_desc";
		this.label_params_desc.Xalign = 0F;
		this.table1.Add(this.label_params_desc);
		global::Gtk.Table.TableChild w11 = ((global::Gtk.Table.TableChild)(this.table1[this.label_params_desc]));
		w11.TopAttach = ((uint)(2));
		w11.BottomAttach = ((uint)(3));
		w11.LeftAttach = ((uint)(2));
		w11.RightAttach = ((uint)(4));
		w11.XOptions = ((global::Gtk.AttachOptions)(4));
		w11.YOptions = ((global::Gtk.AttachOptions)(4));
		// Container child table1.Gtk.Table+TableChild
		this.label_port = new global::Gtk.Label();
		this.label_port.Name = "label_port";
		this.label_port.Xalign = 0F;
		this.label_port.LabelProp = global::Mono.Unix.Catalog.GetString("Port:");
		this.table1.Add(this.label_port);
		global::Gtk.Table.TableChild w12 = ((global::Gtk.Table.TableChild)(this.table1[this.label_port]));
		w12.XOptions = ((global::Gtk.AttachOptions)(0));
		w12.YOptions = ((global::Gtk.AttachOptions)(4));
		this.Add(this.table1);
		if ((this.Child != null))
		{
			this.Child.ShowAll();
		}
		this.DefaultWidth = 670;
		this.DefaultHeight = 397;
		this.combobox_cmd.HasDefault = true;
		this.Show();
		this.DeleteEvent += new global::Gtk.DeleteEventHandler(this.OnDeleteEvent);
		this.comboboxentry_port.Changed += new global::System.EventHandler(this.OnComboBoxEntryPort_Changed);
		this.comboboxentry_params.Changed += new global::System.EventHandler(this.OnComboBoxEntryParams_Changed);
		this.combobox_cmd.Changed += new global::System.EventHandler(this.OnComboBoxCmd_Changed);
		this.button_send.Clicked += new global::System.EventHandler(this.OnButtonSend_Clicked);
		this.button_connect.Clicked += new global::System.EventHandler(this.OnButtonConnect_Clicked);
	}
}
