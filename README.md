
# Resumo
<p align=" justify">Devido ao grande volume de dados, a <a href="https://www.amazon.com.br/Introduction-Parallel-Programming-Peter-Pacheco/dp/0123742609">programação paralela</a> possa ser uma solução. Esse trabalho desenvolveu e implementou versões paralelas e tolerante a falhas do algoritmo de ordenação sequencial <a href="https://pt.wikipedia.org/wiki/Quicksort"> Quicksort</a>.
</p>


# Objetivos
<ul>
  <li>Desenvolver e implementar três versões paralelas do algoritmo  <a href="https://pt.wikipedia.org/wiki/Quicksort">Quicksort</a></li>
  <li>Desenvolver algoritmos tolerante a falhas.</li>
  <li>Ordenar 1 bilhão de números inteiros</li>
  <li>Testar o desempenho para 4, 8, 16 e 32 processos/processadores</li>
</ul>

# Tecnologias
  <ul>
    <li>Linguagem C</li>
    <li>Biblioteca <a href="https://mpitutorial.com/tutorials/mpi-introduction/">MPI</a> para comunicação entre processos paralelos</li>
    <li>Biblioteca <a href="https://fault-tolerance.org/2017/11/03/ulfm-2-0/">ULFM 2.0 (MPI-Forum 2019)</a> para tolerância a falhas</li>
  </ul>
  
# Modelos e definições
  <p align=" justify">
  As trẽs versões paralelas utilizam o conceito de <b>Hipercubo</b> descrito no livro de <a  href="https://www.amazon.com/Introduction-Parallel-Processing-Algorithms-Architectures/dp/B01FKU1TLI"> Parhami</a>
 . Cada processo é modelado como um vértice do hipercubo. A comunicação ponto-a-ponto e a comunicação coletiva entre os processos é realizado com as funções definidas pela biblioteca <a href="https://mpitutorial.com/tutorials/mpi-introduction/">MPI</a>. A biblioteca <a href="https://fault-tolerance.org/2017/11/03/ulfm-2-0/">ULFM 2.0 </a> encarrega-se de tratar as n-1 falhas possíveis para um hipercubo de n vértices. As trẽs versões paralelas desenvolvidas são o <a href="https://github.com/FelipeCamargoXavier/Ordenacao-Paralela/tree/master/hyperquicksort">Hiperquicksort</a>, <a href="https://github.com/FelipeCamargoXavier/Ordenacao-Paralela/tree/master/quickmerge">Quickmerge</a> e <a href="https://github.com/FelipeCamargoXavier/Ordenacao-Paralela/tree/master/modified-quickmerge">Quickmerge modificado</a>.
 </p>
 <p align=" justify">Informações detalhadas podem ser encontradas no <a href="https://drive.google.com/file/d/1NqbbcJ5fusm7YkZqw87N-kp64OgEVfee/view?usp=sharing">artigo</a> publicado na <a href="http://wscad.facom.ufms.br/">WSCAD 2019.</a></p>
 
 # Resultados
 <p align=" justify">
  Os experimentos foram executados em uma máquina com 32 processadores Intel Core i7. O sistema operacional é o Linux Kernel 4.4.0. O desempenho dos algoritmos foi comparado em um cenário sem falhas e com falhas para 4, 8, 16 e 32 processos MPI. Resultados de desempenho mostram a eficiência dos algoritmos para ordenar 1073741824 números inteiros em cenários com falhas e sem falhas. O Hyperquicksort obteve o melhor desempenho, a diferença de desempenho entre o Quickmerge Modificado e o Hyperquicksort é pequena. O Quickmerge Modificado obteve o melhor balanceamento. Nos experimentos com falhas os algoritmos se assemelham em tempo de execução conforme aumenta o número de falhas. 
 </p>
