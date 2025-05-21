<?php
include 'conexao.php';

// Fetch log_abertura_caixa entries with user and medication names
$sql_box_log = "SELECT l.id_log, l.data_hora, l.status, u.nome AS usuario, m.nome AS medicamento
                FROM log_abertura_caixa l
                LEFT JOIN usuario u ON l.id_usuario = u.id_usuario
                LEFT JOIN medicamento m ON l.id_medicamento = m.id_medicamento
                ORDER BY l.data_hora DESC
                LIMIT 50";

$result_box_log = $conn->query($sql_box_log);

// Fetch log_usuario entries with user names
$sql_user_log = "SELECT l.id_log, l.data_hora, u.nome AS usuario, l.acao
                 FROM log_usuario l
                 LEFT JOIN usuario u ON l.id_usuario = u.id_usuario
                 ORDER BY l.data_hora DESC
                 LIMIT 50";

$result_user_log = $conn->query($sql_user_log);
?>

<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8" />
    <title>Monitoramento do Log do Sistema</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="stylesheet" href="style.css" />
</head>
<body>
    <h1>Monitoramento do Log do Sistema</h1>

    <section>
        <h2>Log de Abertura da Caixa</h2>
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Data e Hora</th>
                    <th>Status</th>
                    <th>Usuário</th>
                    <th>Medicamento</th>
                </tr>
            </thead>
            <tbody>
                <?php
                if ($result_box_log && $result_box_log->num_rows > 0) {
                    while ($row = $result_box_log->fetch_assoc()) {
                        echo "<tr>";
                        echo "<td>" . htmlspecialchars($row['id_log']) . "</td>";
                        echo "<td>" . htmlspecialchars($row['data_hora']) . "</td>";
                        echo "<td>" . htmlspecialchars($row['status']) . "</td>";
                        echo "<td>" . htmlspecialchars($row['usuario']) . "</td>";
                        echo "<td>" . htmlspecialchars($row['medicamento']) . "</td>";
                        echo "</tr>";
                    }
                } else {
                    echo "<tr><td colspan='5'>Nenhum registro encontrado.</td></tr>";
                }
                ?>
            </tbody>
        </table>
    </section>

    <section>
        <h2>Log de Ações do Usuário</h2>
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Data e Hora</th>
                    <th>Usuário</th>
                    <th>Ação</th>
                </tr>
            </thead>
            <tbody>
                <?php
                if ($result_user_log && $result_user_log->num_rows > 0) {
                    while ($row = $result_user_log->fetch_assoc()) {
                        echo "<tr>";
                        echo "<td>" . htmlspecialchars($row['id_log']) . "</td>";
                        echo "<td>" . htmlspecialchars($row['data_hora']) . "</td>";
                        echo "<td>" . htmlspecialchars($row['usuario']) . "</td>";
                        echo "<td>" . htmlspecialchars($row['acao']) . "</td>";
                        echo "</tr>";
                    }
                } else {
                    echo "<tr><td colspan='4'>Nenhum registro encontrado.</td></tr>";
                }
                ?>
            </tbody>
        </table>
    </section>
</body>
</html>

<?php
$conn->close();
?>
