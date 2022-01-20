#ifndef ALARM_H
#define ALARM_H

void typing_passwd(char cursor_pos); // The function which shows the procedure of entering the password on the LCD1602, unset alarm 
char enter_passwd(void); // This function is using to entering the admin password
char enter_user_passwd(void); // This function is using to entering the user password
void armed_admin(void); // The function that is responsible for arming our alarm as a administrator. Only the administrator can deactivate alarm
void armed_user(void); // The function that is responsible for arming our alarm as a user. Administrator and user can deactivate alarm
char* change_passwd(char pass[]); // This function is responsible for changing password
void admin_setup(void); // The function allows you to go to the administrator menu
void user_setup(void); // The function allows you to go to the user menu
void alarm(void); // Alarm signaling function

#endif
