#include<iostream>
#include<string>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<map>


// ���������
const int MAX_CONN = 1024;

//����ͻ��˵���Ϣ
struct Client 
{
	int sockfd;
	std::string name;//����

};


int main()
{
	
	//����������socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	{
		perror("socket error");
		return -1;
	}

	// �󶨱���Ip�Ͷ˿�
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// your port
	addr.sin_port = htons(1111);

	int ret = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) 
	{
		printf("bind error\n");
		return -1;
	}


	//�����ͻ���
	ret = listen(sockfd, 1024);
	if (ret < 0) 
	{
		printf("listen error\n");
		return -1;
	}
	//����epollʵ��
	int epld = epoll_create1(0);
	if (epld < 0)
	{
		perror("epoll create error");
		return -1;
	}
	//��������socket����epoll
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;

	ret = epoll_ctl(epld, EPOLL_CTL_ADD, sockfd, &ev);
	if (ret < 0) 
	{
		printf("epoll_ctl error\n");
		return -1;
	}

	//����ͻ�����Ϣ
	std::map<int, Client> clients;

	//ѭ������
	while (1) 
	{
		struct epoll_event evs[MAX_CONN];
		int n = epoll_wait(epld, evs, MAX_CONN, -1);//����
		if (n < 0) 
		{
			printf("epoll_wait error\n");
			break;
		}

		for (int i = 0; i < n; i++) 
		{
			int fd = evs[i].data.fd;
			//����Ǽ�����fd�յ���Ϣ����ô��ʾ�пͻ��˽���������
			if (fd == sockfd) 
			{
				struct sockaddr_in client_addr;
				socklen_t client_addr_len = sizeof(client_addr);
				int client_sockfd = accept(sockfd, (struct sockaddr*) & client_addr, &client_addr_len);
				if (client_sockfd < 0) 
				{
					printf("accept error\n");
					continue;
				}
				//���ͻ��˵�socket����epoll
				struct epoll_event ev_client;
				ev_client.events = EPOLLIN;	//���ͻ�����û����Ϣ����
				ev_client.data.fd = client_sockfd;
				ret = epoll_ctl(epld, EPOLL_CTL_ADD, client_sockfd, &ev_client);
				if (ret < 0)
				{
					printf("epoll_ctl error\n");
					break;
				}
				//printf("%s��������...\n", client_addr.sin_addr.s_addr);


				//����ÿͻ��˵���Ϣ
				Client client;
				client.sockfd = client_sockfd;
				client.name = "";

				clients[client_sockfd] = client;
			}
			else //����ǿͻ�����Ϣ
			{
				char buffer[1024];
				int n = read(fd, buffer, 1024);

				if (n < 0) 
				{
					break;
				}
				else if(n == 0)
				{
					//�ͻ��˶Ͽ�����
					close(fd);
					epoll_ctl(epld, EPOLL_CTL_DEL, fd, 0);

					clients.erase(fd);

				}
				else 
				{
					std::string msg(buffer, n);
					
					//����ÿͻ���nameΪ��˵������Ϣ������ͻ��˵��û���
					if (clients[fd].name == "") 
					{
						clients[fd].name = msg;
					}
					else //������������Ϣ 
					{
						std::string name = clients[fd].name;
						//����Ϣ�����������пͻ���
						for (auto &c : clients) 
						{
							if (c.first != fd) 
							{
								write(c.first, ('[' + name + ']' + ": " + msg).c_str(), msg.size() + name.size() + 4);
							}
						}
					}
				}

			}
		}
	}
	//�ر�epollʵ��
	close(epld);
	close(sockfd);
}