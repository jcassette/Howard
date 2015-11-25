#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPixmap>
#include <QIcon>

#include "commandegen.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    int get_power_value() ;
    int get_times_value() ;
    int get_status_value();

    ~MainWindow();

public slots :
    void process_gauche() ;
    void process_droite() ;
    void process_taking_off() ;
    void process_landing() ;
    void process_monter() ;
    void process_descendre();
    void process_avancer() ;
    void process_reculer() ;
    void process_trans_right() ;
    void process_trans_left() ;
    void process_emerg_stop() ;
    void process_no_emerg_stop() ;
    void process_close() ;
    void process_layout();


private:
    Ui::MainWindow *ui;

    QPushButton *b_haut ;
    QPushButton *b_bas ;
    QPushButton *b_gauche ;
    QPushButton *b_droite ;
    QPushButton *b_take_off;
    QPushButton *b_land;
    QPushButton *b_translate_left ;
    QPushButton *b_translate_right ;
    QPushButton *b_close ;
    QPushButton *b_emergency_stop;
    QPushButton *b_start;
    QPushButton *b_back;

    QSlider *power_slider;
    QSpinBox *e_num_times ;
    QLabel *l_image;
    QLabel *l_img_acceuil ;
    QPixmap *p_image;
    QPixmap *p_img_acceuil;
    QIcon *start_icon;
    QLabel *l_background;
    QPixmap *p_background;



    QGridLayout *layout;
    QVBoxLayout *v_layout;
    QHBoxLayout *h_layout;
    QFormLayout *f_layout;
    QVBoxLayout *acceuil_layout;
    QGridLayout *background_layout;

    QWidget *zoneCentrale;
    QWidget *zoneAcceuil;
    QWidget *zone;

    CommandeGen *gestion_commande ;

    int m_percent ;
    int m_times ;
    int m_status ;
    int m_acceuil;

    void set_m_percent(int percent=-1) ;
    void set_m_times(int times=-1) ;
};

#endif // MAINWINDOW_H