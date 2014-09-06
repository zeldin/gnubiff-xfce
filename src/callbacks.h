
#define CALLBACK_SYMBOL(s) #s, (GCallback)s

extern "C" {

    /* gui.cc */
    gboolean GUI_on_delete_event (GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer data);
    gboolean GUI_on_destroy_event (GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer data);
    void GUI_on_ok (GtkWidget *widget,
                    gpointer data);
    void GUI_on_apply (GtkWidget *widget,
                       gpointer data);
    void GUI_on_close (GtkWidget *widget,
                       gpointer data);
    void GUI_on_cancel (GtkWidget *widget,
                        gpointer data);

#define GUI_CALLBACKS                           \
    CALLBACK_SYMBOL(GUI_on_delete_event),       \
    CALLBACK_SYMBOL(GUI_on_destroy_event),      \
    CALLBACK_SYMBOL(GUI_on_ok),                 \
    CALLBACK_SYMBOL(GUI_on_apply),              \
    CALLBACK_SYMBOL(GUI_on_close),              \
    CALLBACK_SYMBOL(GUI_on_cancel)


    /* ui-applet-gtk.cc */
    void APPLET_GTK_on_enter (GtkWidget *widget,
                              GdkEventCrossing *event,
                              gpointer data);
    gboolean APPLET_GTK_on_button_press (GtkWidget *widget,
                                         GdkEventButton *event,
                                         gpointer data);
    void APPLET_GTK_on_menu_command (GtkWidget *widget,
                                     gpointer data);
    void APPLET_GTK_on_menu_mark (GtkWidget *widget,
                                  gpointer data);
    void APPLET_GTK_on_menu_preferences (GtkWidget *widget, gpointer data);
    void APPLET_GTK_on_menu_about (GtkWidget *widget, gpointer data);
    void APPLET_GTK_on_menu_quit (GtkWidget *widget,
                                  gpointer data);

#define APPLET_GTK_CALLBACKS                            \
    CALLBACK_SYMBOL(APPLET_GTK_on_enter),               \
    CALLBACK_SYMBOL(APPLET_GTK_on_button_press),        \
    CALLBACK_SYMBOL(APPLET_GTK_on_menu_command),        \
    CALLBACK_SYMBOL(APPLET_GTK_on_menu_mark),           \
    CALLBACK_SYMBOL(APPLET_GTK_on_menu_preferences),    \
    CALLBACK_SYMBOL(APPLET_GTK_on_menu_about),          \
    CALLBACK_SYMBOL(APPLET_GTK_on_menu_quit)


    /* ui-popup.cc */
    gboolean POPUP_on_popdown (gpointer data);
    gboolean POPUP_on_button_press (GtkWidget *widget,
                                    GdkEventButton *event,
                                    gpointer data);
    gboolean POPUP_on_button_release (GtkWidget *widget,
                                      GdkEventButton *event,
                                      gpointer data);
    void POPUP_on_enter (GtkWidget *widget,
                         GdkEventCrossing *event,
                         gpointer data);
    void POPUP_on_leave (GtkWidget *widget,
                         GdkEventCrossing *event,
                         gpointer data);
    void POPUP_on_select (GtkTreeSelection *selection, gpointer data);
    void POPUP_menu_message_delete (GtkWidget *widget, gpointer data);
    void POPUP_menu_message_hide (GtkWidget *widget, gpointer data);
    void POPUP_menu_message_undelete (GtkWidget *widget, gpointer data);

#define POPUP_CALLBACKS                                 \
    CALLBACK_SYMBOL(POPUP_on_popdown),                  \
    CALLBACK_SYMBOL(POPUP_on_button_press),             \
    CALLBACK_SYMBOL(POPUP_on_button_release),           \
    CALLBACK_SYMBOL(POPUP_on_enter),                    \
    CALLBACK_SYMBOL(POPUP_on_leave),                    \
    CALLBACK_SYMBOL(POPUP_on_select),                   \
    CALLBACK_SYMBOL(POPUP_menu_message_delete),         \
    CALLBACK_SYMBOL(POPUP_menu_message_hide),           \
    CALLBACK_SYMBOL(POPUP_menu_message_undelete)


    /* ui-preferences.cc */
    gboolean PREFERENCES_on_click (GtkWidget *widget,
                                   GdkEventButton *event,
                                   gpointer data);
    void PREFERENCES_on_add (GtkWidget *widget, gpointer data);
    void PREFERENCES_on_remove (GtkWidget *widget,
                                gpointer data);
    void PREFERENCES_on_properties (GtkWidget *widget,
                                    gpointer data);
    void PREFERENCES_on_stop (GtkWidget *widget,
                              gpointer data);
    void PREFERENCES_on_browse_newmail_image (GtkWidget *widget,
                                              gpointer data);
    void PREFERENCES_on_browse_nomail_image (GtkWidget *widget,
                                             gpointer data);
    void PREFERENCES_on_selection_changed (GtkTreeSelection *selection,
                                           gpointer data);
    void PREFERENCES_on_check_changed (GtkWidget *widget,
                                       gpointer data);
    void PREFERENCES_on_Notebook_switch_page (GtkNotebook *widget,
                                              GtkWidget *page,
                                              gint page_num, gpointer data);
    void PREFERENCES_on_selection_expert (GtkTreeSelection *selection,
                                          gpointer data);
    void PREFERENCES_expert_on_row_activated (GtkTreeView *treeview,
                                              GtkTreePath *path,
                                              GtkTreeViewColumn *col,
                                              gpointer data);
    void PREFERENCES_expert_option_edited  (GtkCellRendererText *cell,
                                            gchar *path_string,
                                            gchar *new_text, gpointer data);
    void PREFERENCES_expert_reset (GtkWidget *widget, gpointer data);
    void PREFERENCES_expert_toggle_option (GtkWidget *widget, gpointer data);
    void PREFERENCES_expert_edit_value (GtkWidget *widget, gpointer data);
    void PREFERENCES_expert_search (GtkWidget *widget, gpointer data);
    void PREFERENCES_expert_new (GtkWidget *widget, gpointer data);
    gboolean PREFERENCES_expert_on_button_press (GtkWidget *widget,
                                                 GdkEventButton *event,
                                                 gpointer data);

#define PREFERENCES_CALLBACKS                                   \
    CALLBACK_SYMBOL(PREFERENCES_on_click),                      \
    CALLBACK_SYMBOL(PREFERENCES_on_add),                        \
    CALLBACK_SYMBOL(PREFERENCES_on_remove),                     \
    CALLBACK_SYMBOL(PREFERENCES_on_properties),                 \
    CALLBACK_SYMBOL(PREFERENCES_on_stop),                       \
    CALLBACK_SYMBOL(PREFERENCES_on_browse_newmail_image),       \
    CALLBACK_SYMBOL(PREFERENCES_on_browse_nomail_image),        \
    CALLBACK_SYMBOL(PREFERENCES_on_selection_changed),          \
    CALLBACK_SYMBOL(PREFERENCES_on_check_changed),              \
    CALLBACK_SYMBOL(PREFERENCES_on_Notebook_switch_page),       \
    CALLBACK_SYMBOL(PREFERENCES_on_selection_expert),           \
    CALLBACK_SYMBOL(PREFERENCES_expert_on_row_activated),       \
    CALLBACK_SYMBOL(PREFERENCES_expert_option_edited),          \
    CALLBACK_SYMBOL(PREFERENCES_expert_reset),                  \
    CALLBACK_SYMBOL(PREFERENCES_expert_toggle_option),          \
    CALLBACK_SYMBOL(PREFERENCES_expert_edit_value),             \
    CALLBACK_SYMBOL(PREFERENCES_expert_search),                 \
    CALLBACK_SYMBOL(PREFERENCES_expert_new),                    \
    CALLBACK_SYMBOL(PREFERENCES_expert_on_button_press)


    /* ui-properties.cc */
    void PROPERTIES_on_delay (GtkWidget *widget,
                              gpointer data);
    void PROPERTIES_on_port (GtkWidget *widget,
                             gpointer data);
    void PROPERTIES_on_mailbox (GtkWidget *widget,
                                gpointer data);
    void PROPERTIES_on_type_changed (GtkComboBoxText *cbox,
                                     gpointer   data);
    void PROPERTIES_on_auth_changed (GtkComboBoxText *cbox,
                                     gpointer   data);
    void PROPERTIES_on_browse_address (GtkWidget *widget,
                                       gpointer data);
    void PROPERTIES_on_browse_certificate (GtkWidget *widget,
                                           gpointer data);

#define PROPERTIES_CALLBACKS                            \
    CALLBACK_SYMBOL(PROPERTIES_on_delay),               \
    CALLBACK_SYMBOL(PROPERTIES_on_port),                \
    CALLBACK_SYMBOL(PROPERTIES_on_mailbox),             \
    CALLBACK_SYMBOL(PROPERTIES_on_type_changed),        \
    CALLBACK_SYMBOL(PROPERTIES_on_auth_changed),        \
    CALLBACK_SYMBOL(PROPERTIES_on_browse_address),      \
    CALLBACK_SYMBOL(PROPERTIES_on_browse_certificate)

}

#define ALL_CALLBACKS                           \
    GUI_CALLBACKS,                              \
    APPLET_GTK_CALLBACKS,                       \
    POPUP_CALLBACKS,                            \
    PREFERENCES_CALLBACKS,                      \
    PROPERTIES_CALLBACKS
