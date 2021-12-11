#ifndef ALARM_H
#define ALARM_H

void typing_passwd(char cursor_pos); // The function which shows the procedure of entering the password on the LCD1602, unset alarm 
char enter_passwd(void); // This function is using to entering the password
void armed(void); // The function that is responsible for arming our alarm
char* change_passwd(char pass[]); // This function is responsible for changing password
void admin_setup(void); // The function allows you to go to the administrator menu 
void alarm(void); // Alarm signaling function

#endif
