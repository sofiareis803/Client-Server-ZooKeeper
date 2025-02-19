Utilizamos os ficheiros objeto disponibilizados pelo professor para
a fase 3, por isso temos os .o's no diretório 'object'.

---

Para desligar os clientes quando não houvessem servidores ativos,
adotamos uma implementação pouco usual usando polls, para que o programa não 
ficasse travado no fgets (acreditamos que esse era o problema) e pudesse terminar

---

Para executar o server_hashtable é preciso dar:
  - o ip:porto do zookeeper
  - o porto do servidor
  - o numero de listas
nessa ordem
