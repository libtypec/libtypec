#include <gtk/gtk.h>
#include "../libtypec.h"
#include "lstypec.h"
#include "names.h"

void show_error_dialog(const gchar *message);

GtkTextBuffer *txt_buffer;
// Function to create the tree store
GtkTreeStore *create_tree_store() {
  int ret;
  GtkTreeStore *store = gtk_tree_store_new(1, G_TYPE_STRING);

  GtkTreeIter parent, child;

  names_init();

  // Initialize libtypec by default with sysfs and print session info
  ret = libtypec_init(session_info,LIBTYPEC_BACKEND_SYSFS);

  if (ret < 0)
  {
    show_error_dialog("Failed in Initializing libtypec");
    exit(1);
  }

  // Level 1
  gtk_tree_store_append(store, &parent, NULL);
  gtk_tree_store_set(store, &parent, 0, "USB-C/PD Platform Policy", -1);


  // PPM Capabilities
  ret = libtypec_get_capability(&get_cap_data);

  if (ret < 0)
  {
    show_error_dialog("Failed in Get Capability");
    exit(1);
  }

  for (int i = 0; i < get_cap_data.bNumConnectors; i++) 
  {
    char string[255]; 
    gtk_tree_store_append(store, &child, &parent);
    sprintf (string, "Port: %d", i);
    gtk_tree_store_set(store, &child, 0, string, -1);
    
  }

  return store;
}
void build_vdo(uint32_t vdo, int num_fields, const struct vdo_field vdo_fields[], const char *vdo_field_desc[][MAX_FIELDS])
{
  char val[1024];
  for (int i = 0; i < num_fields; i++) {
    if (!vdo_fields[i].print)
      continue;

    uint32_t field = (vdo >> vdo_fields[i].index) & vdo_fields[i].mask;
    sprintf(val,"      %s: %*d", vdo_fields[i].name, FIELD_WIDTH(MAX_FIELD_LENGTH - ((int) strlen(vdo_fields[i].name))), ((vdo >> vdo_fields[i].index) & vdo_fields[i].mask));
    gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

    if (vdo_field_desc[i][0] != NULL) {
      // decode field
      sprintf(val," (%s)\n", vdo_field_desc[i][field]);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

    } else if (strcmp(vdo_fields[i].name, "USB Vendor ID")  == 0) {
      // decode vendor id
       char vendor_str[128];
       uint16_t svid = ((vdo >> vdo_fields[i].index) & vdo_fields[i].mask);
       get_vendor_string(vendor_str, sizeof(vendor_str), svid);
      sprintf(val," (%s)\n", (vendor_str[0] == '\0' ? "unknown" : vendor_str));
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

    } else {
      // No decoding
      sprintf(val,"\n");
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

    }
  }
}

void build_conn_capability(struct libtypec_connector_cap_data conn_data)
{
  char *opr_mode_str[] = {"Rp Only", "Rd Only", "DRP(Rp/Rd)", "Analog Audio", "Debug Accessory", "USB2", "USB3", "Alternate Mode"};
  char val[512];

  sprintf(val,"  Operation Modes Supported: 0x%02x ", conn_data.opr_mode.raw_operationmode);
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

    for (int i = 0; i < sizeof(opr_mode_str) / sizeof(opr_mode_str[0]); i++) {
      if (conn_data.opr_mode.raw_operationmode & (1 << i)) {
        sprintf(val, "(%s)", opr_mode_str[i]);
        gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));
      }
    }
	sprintf(val,"\n");
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

}
void get_svid_string(uint32_t svid, char* str) {

    switch (svid) {
        case 0xFF01:
            strcpy(str, "Display Alternate Mode");
            break;
        case 0x8087:
            strcpy(str, "TBT Alternate Mode");
            break;
        default:
            get_vendor_string(str, sizeof(str), svid);
            break;
    }
}

void  build_alternate_mode_data(int recipient, uint32_t id_header, int num_modes, struct altmode_data *am_data)
{
  char vendor_id[128];
  char val[512];

  if (recipient == AM_CONNECTOR) {
    for (int i = 0; i < num_modes; i++) {
      get_svid_string(am_data[i].svid,vendor_id);
      sprintf(val,"  Local Mode %d:\n", i);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      sprintf(val,"    SVID: 0x%04x (%s)\n", am_data[i].svid,vendor_id);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      sprintf(val,"    VDO: 0x%08x\n", am_data[i].vdo);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      switch(am_data[i].svid){
        case 0x8087:
          build_vdo(am_data[i].vdo, 7, tbt3_sop_fields, tbt3_sop_field_desc);
          break;
        case 0xff01:
          build_vdo(am_data[i].vdo, 7, dp_alt_mode_partner_fields, dp_alt_mode_partner_field_desc);
          break;
        default:
          get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
          sprintf(val,"      VDO Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
          gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));
          break;
      }
    }
  }

  if (recipient == AM_SOP) {
    for (int i = 0; i < num_modes; i++) {
      get_svid_string(am_data[i].svid,vendor_id);
      sprintf(val,"  Partner Mode %d:\n", i);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      sprintf(val,"    SVID: 0x%04x (%s)\n", am_data[i].svid,vendor_id);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      sprintf(val,"    VDO: 0x%08x\n", am_data[i].vdo);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      switch(am_data[i].svid){
      case 0x8087:
        build_vdo(am_data[i].vdo, 7, tbt3_sop_fields, tbt3_sop_field_desc);
        break;
      case 0xff01:
        build_vdo(am_data[i].vdo, 7, dp_alt_mode_partner_fields, dp_alt_mode_partner_field_desc);
        break;
      default:
        get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
        sprintf(val,"      VDO Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
        gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

        break;
      }
    }
  }

  if (recipient == AM_SOP_PR) {
    for (int i = 0; i < num_modes; i++) {
      get_svid_string(am_data[i].svid,vendor_id);
      sprintf(val,"  Cable Plug Modes %d:\n", i);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      sprintf(val,"    SVID: 0x%04x (%s)\n", am_data[i].svid,vendor_id);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      sprintf(val,"    VDO: 0x%08x\n", am_data[i].vdo);
      gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

      switch(am_data[i].svid){
      case 0x8087:
        build_vdo(am_data[i].vdo, 7, tbt3_sop_pr_fields, tbt3_sop_pr_field_desc);
        break;
      case 0xff01:
        if ((id_header & ACTIVE_CABLE_MASK) == ACTIVE_CABLE_COMP) {
          build_vdo(am_data[i].vdo, 7, dp_alt_mode_active_cable_fields, dp_alt_mode_active_cable_field_desc);
        } else {
          get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
          sprintf(val,"      SVID Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
          gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

        }
        break;
      default:
        get_vendor_string(vendor_id, sizeof(vendor_id), am_data[i].svid);
        sprintf(val,"      SVID Decoding not supported for 0x%04x (%s)\n", am_data[i].svid, (vendor_id[0] == '\0' ? "unknown" : vendor_id));
        gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

        break;
      }
    }
  }
}
void build_cable_prop(struct libtypec_cable_property cable_prop, int conn_num)
{
    char *cable_type[] = {"Passive", "Active", "Unknown"};
    char *cable_plug_type[] = {"USB Type A", "USB Type B", "USB Type C", "Non-USB Type", "Unknown"};
    char val[512];

    sprintf(val,"  Cable Property in Port %d:\n", conn_num);
    gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

    sprintf(val,"    Cable Type: %s\n", cable_type[cable_prop.cable_type]);
    gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

    sprintf(val,"    Cable Plug Type: %s\n", cable_plug_type[cable_prop.plug_end_type]);
    gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

}

void build_capabilities_partner(int i)
{
    int num_modes;

   // Partner
    num_modes = libtypec_get_alternate_modes(AM_SOP, i, am_data);
    if (num_modes >= 0) 
      build_alternate_mode_data(AM_SOP, id.disc_id.id_header, num_modes, am_data);
}
void build_capabilities_cable(int i)
{
    int ret, num_modes;

     // Resetting port properties
    cable_prop.cable_type = CABLE_TYPE_PASSIVE;
    cable_prop.plug_end_type = PLUG_TYPE_OTH;
   // Cable Properties
    
    ret = libtypec_get_cable_properties(i, &cable_prop);
    if (ret >= 0)
      build_cable_prop(cable_prop, i);

        // Cable
    num_modes = libtypec_get_alternate_modes(AM_SOP_PR, i, am_data);
    if (num_modes >= 0) 
      build_alternate_mode_data(AM_SOP_PR, id.disc_id.id_header, num_modes, am_data);
}

void build_capabilities_port(int i)
{
  int num_modes;
  char val[512];


  // Connector Capabilities
	sprintf(val,"\nConnector %d Capability/Status\n", i);
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

  libtypec_get_conn_capability(i, &conn_data);
  build_conn_capability(conn_data);

  // Supported Alternate Modes
  sprintf(val,"  Alternate Modes Supported:\n");
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

  num_modes = libtypec_get_alternate_modes(AM_CONNECTOR, i, am_data);
  if (num_modes > 0)
    build_alternate_mode_data(AM_CONNECTOR, 0x0, num_modes, am_data);
  else {
    sprintf(val,"    No Local Modes listed with typec class\n");
    gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));
  }
}

void build_ppm_capability(struct libtypec_capability_data ppm_data)
{
  char val[512];
  sprintf(val,"\nUSB-C Platform Policy Manager Capability\n");
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

  sprintf(val,"  Number of Connectors: %x\n", ppm_data.bNumConnectors);
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

  sprintf(val,"  Number of Alternate Modes: %x\n", ppm_data.bNumAltModes);
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

  sprintf(val,"  USB Power Delivery Revision: %x.%x\n", (ppm_data.bcdPDVersion >> 8) & 0XFF, (ppm_data.bcdPDVersion) & 0XFF);
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

  sprintf(val,"  USB Type-C Revision:  %x.%x\n",(ppm_data.bcdTypeCVersion >> 8) & 0XFF, (ppm_data.bcdTypeCVersion) & 0XFF);
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

  sprintf(val,"  USB BC Revision: %x.%x\n", (ppm_data.bcdBCVersion >> 8) & 0XFF, (ppm_data.bcdBCVersion) & 0XFF);
  gtk_text_buffer_insert_at_cursor(txt_buffer, val,strlen(val));

}

// Function to update the text buffer based on the selected tree item
void on_tree_selection_changed(GtkTreeSelection *selection, gpointer data) {
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *text;
  GtkTextIter begin;
  GtkTextIter end;
  int num;

  gtk_text_buffer_get_start_iter(txt_buffer,&begin);
	gtk_text_buffer_get_end_iter(txt_buffer,&end);
	gtk_text_buffer_delete (txt_buffer, &begin, &end);

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 0, &text, -1);

    if (strstr(text, "Port") != NULL) {
      char *p = strchr(text,' ');
      sscanf(p,"%d",&num);
      build_capabilities_port(num);
      build_capabilities_cable(num);
      build_capabilities_partner(num);
    } else {
      build_ppm_capability(get_cap_data);
    }
    
    g_free(text);
  }
}

void show_error_dialog(const gchar *message) {
  GtkWidget *dialog;

  gtk_init (NULL, NULL);

  dialog = gtk_message_dialog_new (NULL,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   "%s", message); 
  gtk_window_set_title (GTK_WINDOW (dialog), "Error");

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);


  // Create the main window
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "USB-C/PD Viewer");
  gtk_window_set_default_size(GTK_WINDOW(window), 1080, 800);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  // Create the main layout (horizontal box)
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_add(GTK_CONTAINER(window), hbox);

  // Create the tree view
  GtkTreeStore *store = create_tree_store();
  GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
      "USB-C/PD - Ports/Partners", renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  // Create the tree selection
  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

  // Create the text buffer
  txt_buffer = gtk_text_buffer_new(NULL);
  GtkWidget *textview = gtk_text_view_new_with_buffer(txt_buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);

  // Connect the tree selection changed signal
  g_signal_connect(selection, "changed",
                   G_CALLBACK(on_tree_selection_changed), txt_buffer);

  // Add the tree view and text view to the main layout
  GtkWidget *scrolled_window_tree =
      gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window_tree), treeview);
  gtk_box_pack_start(GTK_BOX(hbox), scrolled_window_tree, TRUE, TRUE, 0);

  GtkWidget *scrolled_window_text =
      gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window_text), textview);
  gtk_box_pack_start(GTK_BOX(hbox), scrolled_window_text, TRUE, TRUE, 0);

  // Show all widgets
  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
