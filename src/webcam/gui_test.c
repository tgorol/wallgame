#include "include/gui_test.h"
#include <gtk/gtk.h>

enum{
    TEST_SIGNAL,
    LAST_SIGNAL
};

static gint test_signals[LAST_SIGNAL] = {0};

static void
test_class_init(TestClass *wclass)
{
    test_signals[TEST_SIGNAL] = g_signal_new("Test",
            G_TYPE_FROM_CLASS(wclass),
            G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET(TestClass, test),
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
}

static void
test_init(Test *ttt)
{
    ttt->button = gtk_button_new_with_label("Test");

    gtk_box_pack_start(GTK_BOX(ttt), ttt->button, TRUE, TRUE, 0);

    gtk_widget_show(ttt->button);

    return;
}


guint
test_get_type()
{
    static guint ttt_type = 0;
    static GTypeInfo ttt_info = {
        sizeof (TestClass),
        NULL,
        NULL,
        (GClassInitFunc) test_class_init,
        NULL,
        NULL,
        sizeof (Test),
        0,
        (GInstanceInitFunc) test_init
    };

    return ttt_type == 0 ? g_type_register_static(GTK_TYPE_VBOX, "Test",
            &ttt_info, 0) : ttt_type;
}


GtkWidget*
test_new(void)
{
    return GTK_WIDGET(g_object_new(test_get_type(), NULL));
}
