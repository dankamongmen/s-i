Template: netcfg/get_netmask
Type: string
Description: Netmask?
 The netmask is used to determine which machines are local to your network.
  Consult your network administrator if you do not know the value.  The
 netmask should be entered as four numbers separated by periods. 
Description-ru: ������� �����?
 ������� ����� ������������ ��� �����������, ����� ������ ������� ����������
 ��� ����� ����. ������������������� � ����� ������� ���������������, ����
 �� �� ������, ����� ��� ������ ����. ������� ����� �������� ��� ������
 �����, ����������� �������.

Template: netcfg/get_ipaddress
Type: string
Description: IP address?
 The IP address is unique to your computer and consists of four numbers
 separated by periods.  If you don't know what to use here, consult your
 network administrator. 
Description-ru: IP �����?
 IP ����� �������� ��� ������ ���������� � ������� �� ������� �����,
 ����������� �������. ���� �� �� ������, ��� ����� �������, ��
 ������������������� � ����� ������� ���������������.

Template: netcfg/gateway_unreachable
Type: note
Description: The gateway you entered is unreachable.
 You may have made an error entering your ipaddress, netmask and/or
 gateway. 
Description-ru: ��������� ���� ���� ����������.
 �� ����� ��������� ��� ����� ������ IP ������, ������� ����� �/���
 �����.

Template: netcfg/get_gateway
Type: string
Description: Gateway?
 This is an IP address (four numbers separated by periods) that indicates
 the gateway router, also known as the default router.  All traffic that
 goes outside your LAN (for instance, to the Internet) is sent through this
 router.  In rare circumstances, you may have no router; in that case, you
 can leave this blank.  If you don't know the proper answer to this
 question, consult your network administrator. 
Description-ru: ����?
 ��� IP ����� (������ �����, ����������� �������), ������� ��������� ��������
 �������������, ����� ��������� ��� ������������� �� ���������. ���� ������,
 ������� ������� �� ������� ����� ��� (��������, � ��������) ������������ �����
 ���� �������������. � ������ ������� �� ������ �� ����� ������ ��������������;
 � ���� ������ �� ������ �������� ���� ������. ���� �� �� ������ �����������
 ������ �� ���� ������, �� ������������� � ����� ������� ���������������.

Template: netcfg/confirm_static
Type: boolean
Default: true
Description: Is this configuration correct?
Description-ru: ��� ��������� �����?
