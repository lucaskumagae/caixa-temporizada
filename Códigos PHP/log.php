<?php
include 'conexao.php';

$filter_date = isset($_GET['filter_date']) ? $_GET['filter_date'] : '';

// Prepare SQL with optional date filter on buzzer date (date part only)
$sql_box_log = "
SELECT 
    l.data_hora_buzzer AS buzzer,
    l.data_hora_abertura AS abertura_caixa,
    m.nome AS medicamento,
    l.status
FROM log_abertura_caixa l
LEFT JOIN medicamento m ON l.id_medicamento = m.id_medicamento
";

if ($filter_date) {
    $sql_box_log .= " WHERE DATE(l.data_hora_buzzer) = ?";
}

$sql_box_log .= " ORDER BY l.data_hora_abertura DESC LIMIT 50";

if ($filter_date) {
    $stmt = $conn->prepare($sql_box_log);
    $stmt->bind_param("s", $filter_date);
    $stmt->execute();
    $result_box_log = $stmt->get_result();
} else {
    $result_box_log = $conn->query($sql_box_log);
}

function format_status($status) {
    switch ($status) {
        case 'esquecido':
            return 'Esquecido';
        case 'fora de horario':
            return 'Tomado fora do horário';
        case 'tomado corretamente':
            return 'Tomado corretamente';
        default:
            return $status;
    }
}
?>

<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8" />
    <title>Monitoramento do Log do Sistema</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="stylesheet" href="style.css" />
    <style>
        /* Additional styling for filter form */
        .filter-form {
            margin-bottom: 1em;
        }
        .filter-form label {
            font-weight: bold;
            margin-right: 0.5em;
        }
        .filter-form input[type="date"] {
            padding: 0.3em;
            border: 1px solid #ccc;
            border-radius: 4px;
        }
        .filter-form button {
            padding: 0.3em 0.8em;
            margin-left: 0.5em;
            border: none;
            background-color: #007bff;
            color: white;
            border-radius: 4px;
            cursor: pointer;
        }
        .filter-form button:hover {
            background-color: #0056b3;
        }
    </style>
</head>
<body>
    <?php include 'nav.php'; ?>

    <main class="content">
        <header class="header">
            <h1>Monitoramento do Log do Sistema</h1>
        </header>

        <form method="get" class="filter-form">
            <label for="filter_date">Filtrar por data:</label>
            <input type="date" id="filter_date" name="filter_date" value="<?php echo htmlspecialchars($filter_date); ?>" />
            <button type="submit">Filtrar</button>
        </form>

        <section>
            <table>
                <thead>
                    <tr>
                        <th>Horário do Buzzer</th>
                        <th>Horário de Abertura</th>
                        <th>Medicamento</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
                    <?php
                    if ($result_box_log && $result_box_log->num_rows > 0) {
                        while ($row = $result_box_log->fetch_assoc()) {
                            // Format date/time to Brazilian format: dd/mm/yyyy hh:mm:ss
                            $buzzer_dt = new DateTime($row['buzzer']);
                            $formatted_buzzer = $buzzer_dt->format('d/m/Y H:i:s');

                            if ($row['status'] === 'esquecido') {
                                $formatted_open = "∅";
                            } else {
                                $open_dt = new DateTime($row['abertura_caixa']);
                                $formatted_open = $open_dt->format('d/m/Y H:i:s');
                            }

                            $status_display = format_status($row['status']);

                            echo "<tr>";
                            echo "<td>" . htmlspecialchars($formatted_buzzer) . "</td>";
                            echo "<td>" . htmlspecialchars($formatted_open) . "</td>";
                            echo "<td>" . htmlspecialchars($row['medicamento']) . "</td>";
                            echo "<td>" . htmlspecialchars($status_display) . "</td>";
                            echo "</tr>";
                        }
                    } else {
                        echo "<tr><td colspan='4'>Nenhum registro encontrado.</td></tr>";
                    }
                    ?>
                </tbody>
            </table>
        </section>
    </main>
</body>
</html>

<?php
$conn->close();
?>
